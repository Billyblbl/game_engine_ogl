#ifndef GRECT_EDITOR
# define GRECT_EDITOR

#include <application.cpp>
#include <imgui_extension.cpp>
#include <utils.cpp>
#include <physics2d.cpp>
#include <transform.cpp>
#include <textures.cpp>
#include <framebuffer.cpp>
#include <blblstd.hpp>
#include <sprite.cpp>
#include <rendering.cpp>
#include <animation.cpp>

const struct {
	cstrp draw_pipeline = "./shaders/sprite.glsl";
} assets;

struct SpriteObject {
	TexBuffer texture;
	RenderMesh rect;
	MappedObject<m4x4f32> view_projection_matrix = map_object(m4x4f32(1));
};

void unload(SpriteObject& obj) {
	unload(obj.texture);
	delete_mesh(obj.rect);
	unmap(obj.view_projection_matrix);
	obj.texture.id = 0;
}

SpriteObject load(SpriteObject& old, const cstr path, f32 ppu) {
	unload(old);
	auto texture = load_texture(path, RGBA32F, TX2DARR);
	auto rect = create_rect_mesh(texture.dimensions, ppu);
	auto matrix = map_object(m4x4f32(1));
	return { texture, rect, matrix };
}

b2Fixture* reset_fixture_shape(b2Body* body, const b2Shape& new_shape) {
	body->DestroyFixture(body->GetFixtureList());
	b2FixtureDef def;
	def.shape = &new_shape;
	return body->CreateFixture(&def);
}

i32 main() {
	auto app = create_app("BLBLED", v2u32(1920, 1080), null); defer{ destroy(app); };
	if (!init_ogl(app))
		return 1;

	ImGui::init_ogl_glfw(app.window); defer{ ImGui::shutdown_ogl_glfw(); };

	struct {
		OrthoCamera camera = { v3f32(16.f, 9.f, 1000.f) / 2.f, v3f32(0) };
		SpriteRenderer draw = load_sprite_renderer(assets.draw_pipeline, 100);
		SpriteObject sprite;
	} rendering; defer{ unload(rendering.sprite); };
	rendering.sprite.texture.id = 0;

	struct {
		b2World world = b2World(b2Vec2(0.f, -9.f));
		B2dDebugDraw debug_draw;
		b2PolygonShape shape;
	} physics;
	physics.world.SetDebugDraw(&physics.debug_draw);
	physics.debug_draw.SetFlags(b2Draw::e_shapeBit);
	physics.debug_draw.view_transform = &rendering.sprite.view_projection_matrix;

	struct {
		TexBuffer color;
		TexBuffer depth;
		GLuint fb;
		bool active = true;
		bool cam_controls = false;
	} scene;
	scene.color = create_texture(TX2D, v4u32(app.pixel_dimensions, 1, 1), RGBA32F); defer{ unload(scene.color); };
	scene.depth = create_texture(TX2D, v4u32(app.pixel_dimensions, 1, 1), DEPTH_COMPONENT32); defer{ unload(scene.depth); };
	scene.fb = create_framebuffer({ bind_to_fb(Color0Attc, scene.color, 0, 0), bind_to_fb(DepthAttc, scene.depth, 0, 0) }); defer{ destroy_fb(scene.fb); };

	struct {
		bool active;
		b2Body* body = null;
		b2Fixture* shape = null;
		Transform2D transform;
		f32 ppu = 256;
		SpriteCursor sprite = { {v2f32(0), v2f32(1)}, 0 };
	} shape_editor;

	b2BodyDef def_bod;
	def_bod.position = b2Vec2(0, 0);
	def_bod.type = b2_staticBody;
	shape_editor.body = physics.world.CreateBody(&def_bod);
	b2FixtureDef def_fix;
	physics.shape.SetAsBox(1, 1);
	def_fix.shape = &physics.shape;
	shape_editor.shape = shape_editor.body->CreateFixture(&def_fix);

	fflush(stdout);
	wait_gpu();

	while (update(app, null)) {

		// UI
		auto draw_data = ImGui::RenderNewFrame(
			[&]() {
				if (ImGui::BeginMainMenuBar()) {
					defer{ ImGui::EndMainMenuBar(); };
					if (ImGui::BeginMenu("Windows")) {
						defer{ ImGui::EndMenu(); };
						ImGui::MenuItem("Scene", null, &scene.active);
						ImGui::MenuItem("Scene camera controls", null, &scene.cam_controls);
						ImGui::MenuItem("Shape editor", "Alt+S", &shape_editor.active);
					}
				}

				ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

				if (scene.active) {
					ImGui::Begin("Scene"); defer{ ImGui::End(); };
					render(scene.fb, { v2u32(0), scene.color.dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, v4f32(v3f32(0.3), 1),
						[&]() {
							*rendering.sprite.view_projection_matrix.obj = view_project(project(rendering.camera), m4x4f32(1));
							if (rendering.sprite.texture.id != 0) {
								auto batch = rendering.draw.start_batch();
								batch.push(sprite_data(trs_2d(shape_editor.transform), shape_editor.sprite.uv_rect, shape_editor.sprite.atlas_index, 0));
								rendering.draw(rendering.sprite.rect, rendering.sprite.texture, rendering.sprite.view_projection_matrix, 1);
							}
							physics.world.DebugDraw();
						}
					);
					ImGui::Image((ImTextureID)(u64)scene.color.id, ImGui::fit_to_window(ImGui::from_glm(scene.color.dimensions)), ImVec2(0, 1), ImVec2(1, 0));
				}

				if (scene.cam_controls) {
					ImGui::Begin("Camera"); defer{ ImGui::End(); };
					EditorWidget("Orthographic", rendering.camera);
				}

				if (shape_editor.active) {
					ImGui::Begin("Shape"); defer{ ImGui::End(); };

					EditorWidget("Sprite cursor", shape_editor.sprite);
					static char sprite_filename[999] = "sprite_path.png";
					ImGui::DragFloat("Pixel per unit", &shape_editor.ppu);
					ImGui::InputText("Sprite file", sprite_filename, sizeof(sprite_filename));
					if (ImGui::Button("Load Sprite"))
						rendering.sprite = load(rendering.sprite, sprite_filename, shape_editor.ppu);

					EditorWidget("Transform", shape_editor.transform);
					override_body(shape_editor.body, shape_editor.transform.translation, shape_editor.transform.rotation);
					auto* polygon_shape = (b2PolygonShape*)shape_editor.shape->GetShape();
					i32 v_count = polygon_shape->m_count;
					if (ImGui::InputInt("Vertices count", &v_count) && v_count < b2_maxPolygonVertices) {
						b2PolygonShape new_shape = *((b2PolygonShape*)shape_editor.shape->GetShape());
						new_shape.m_count = v_count;
						shape_editor.shape = reset_fixture_shape(shape_editor.body, new_shape);
					}
					EditorWidget("Vertices", cast<v2f32>(carray(polygon_shape->m_vertices, polygon_shape->m_count)));
					auto current_shape = (b2PolygonShape*)shape_editor.shape->GetShape();
					auto convex = current_shape->Validate();
					ImGui::Text("Convex : %s\n", convex ? "true" : "false");

					ImGui::Text("TODO : Path from OS filesystem selection");
					static char polygon_filename[999] = "polygon_path.poly";
					ImGui::InputText("Polygon file", polygon_filename, sizeof(polygon_filename));
					ImGui::BeginDisabled(!convex);
					if (ImGui::Button("Save Poly"))
						save_polygon(polygon_filename, current_shape);
					ImGui::EndDisabled();
					ImGui::SameLine();
					if (ImGui::Button("Load Poly"))
						shape_editor.shape = reset_fixture_shape(shape_editor.body, load_b2d_polygon(polygon_filename));
				}

			}
		);

		render(0, { v2u32(0), app.pixel_dimensions }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, v4f32(v3f32(0), 1), [&]() { ImGui::Draw(draw_data); });
	}
	return 0;
}

#endif
