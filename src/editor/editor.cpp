#include "cglm/types-struct.h"
#include <cstdio>
#define NO_GLAD
#include "./imgui.cpp"
#include "./imgui_demo.cpp"
#include "./imgui_draw.cpp"
#include "./imgui_impl_opengl3.cpp"
#include "./imgui_impl_sdl3.cpp"
#include "./imgui_stdlib.cpp"
#include "./imgui_tables.cpp"
#include "./imgui_widgets.cpp"
#include "./imgui.h"
#include "./imgui_impl_sdl3.h"
#include "./imgui_impl_opengl3.h"

#include "../types.h"
#include "SDL3/SDL_events.h"
#include "../arena.c"

struct arena editor_arena;

void scene_view_draw(struct editor *editor)
{
	ImGui::Begin("scene");

	if (editor->mode == DEFAULT) {
		if (ImGui::Button("Play", ImVec2(40, 25))) {
			editor->mode = PLAY;
			editor->reload_scene = true;
			// editor->game->start_game(editor->game, editor->scene);
		}
	} else if (editor->mode == PLAY) {
		if (ImGui::Button("Stop", ImVec2(40, 25))) {
			editor->mode = DEFAULT;
			editor->reload_scene = true;
		}
	}

	ImVec2 available_space = ImGui::GetContentRegionAvail();
	float aspect = (float)editor->ren->final_fbo.width / editor->ren->final_fbo.height;
	ImVec2 image_size = ImVec2(editor->ren->final_fbo.width, editor->ren->final_fbo.height);
	image_size.y = image_size.y < available_space.y ? image_size.y : available_space.y;
	image_size.x = image_size.y * aspect;

	if (image_size.x > available_space.x) {
		image_size.x = available_space.x;
		image_size.y = image_size.x * (1.0f / aspect);
	}

	ImVec2 image_pos;
	image_pos.x = (available_space.x - image_size.x) / 2.0f;
	image_pos.y = (available_space.y - image_size.y) / 2.0f;

	editor->image_size.x = image_size.x;
	editor->image_size.y = image_size.y;
	editor->image_pos.x = image_pos.x;
	editor->image_pos.y = image_pos.y;
	ImGui::SetCursorPos(image_pos);

	ImGui::Image(editor->ren->final_fbo.id, image_size, ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();
}

void debug_draw(struct editor *editor)
{
	ImGui::Begin("Debug");
	ImGui::DragFloat2("image size", &editor->image_size.x);
	ImGui::DragFloat2("image pos", &editor->image_pos.x);
	ImGui::DragFloat3("light direction", &editor->scene->light_direction.x);
	if (ImGui::Button("Save Scene", ImVec2(150, 80))) {
		editor->ren->scene_write(editor->scene, editor->physics);
	}

	if (ImGui::Button("Reload Shaders", ImVec2(150, 80))) {
		editor->ren->reload_shaders(editor->ren);
	}

	ImGui::DragFloat("spawn velocity", &editor->game->spawn_force);

	if (ImGui::Checkbox("draw physics bodies", &editor->physics->draw_debug)) {
	}

	if (ImGui::Checkbox("SDF Renderer", &editor->ren->sdf_renderer)) {
	}

	ImGui::End();
}

void inspector_draw_add_component(struct editor *editor, struct entity *e)
{
	if (ImGui::BeginCombo("##Add Component", "Add Component")) {
		const bool is_selected = false;
		if (!e->body) {
			if (ImGui::Selectable("Rigidbody - static", is_selected)) {
				e->body = editor->physics->add_rigidbody_box(editor->physics, editor->scene, e, true);
			}

			if (ImGui::Selectable("Rigidbody - dynamic", is_selected)) {
				e->body = editor->physics->add_rigidbody_box(editor->physics, editor->scene, e, false);
			}
		}
		ImGui::EndCombo();
	}
}

void inspector_draw_entity(struct editor *editor, struct entity *e)
{
	char name[128];
	snprintf(name, 128, "%s", e->name);
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;

	if (ImGui::InputTextWithHint("Name", name, name, 128, flags)) {
		if (name[0] != '\0') {
			snprintf(e->name, 128, "%s", name);
		}
	}

	ImGui::SameLine();
	ImGui::Text("%lu", e->id);

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		vec3s euler_angles = e->transform->euler_angles;

		if (ImGui::DragFloat3("pos", &e->transform->pos.x, 0.01f)) {
			editor->game->set_position(e->transform, e->transform->pos);
		}

		if (ImGui::DragFloat3("rot", &euler_angles.x, 0.01f)) {
			editor->game->set_euler_angles(e->transform, euler_angles);
		}

		if (ImGui::DragFloat3("scale", &e->transform->scale.x, 0.01f)) {
			editor->game->set_scale(e->transform, e->transform->scale);
		}
	}

	if (e->camera) {
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			float fov_deg = glm_deg(e->camera->fov);
			ImGui::DragFloat("FOV", &fov_deg, 0.01f);
			ImGui::DragFloat("near", &e->camera->near_plane);
			ImGui::DragFloat("far", &e->camera->far_plane);
			e->camera->fov = glm_rad(fov_deg);
		}
	}

	if (e->renderer) {
		if (ImGui::CollapsingHeader("Renderer Thing", ImGuiTreeNodeFlags_DefaultOpen)) {
			// ImGui::Text("%s", e->renderer->mesh->name);

			if (ImGui::BeginCombo("mesh", e->renderer->mesh->name)) {
				for (int i = 0; i < editor->res->num_meshes; i++) {
					char label[256];
					snprintf(label, 256, "%s%i", editor->res->meshes[i].name, i);
					if (ImGui::Selectable(label)) {
						e->renderer->mesh = &editor->res->meshes[i];
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::Checkbox("is light", &e->renderer->is_lighting)) {
			}
		}

		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::ColorEdit4("base color##3", &e->renderer->mesh->sub_meshes[0].mat->color.r,
					  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
			// ImGui::ColorPicker4("base color", &e->renderer->mesh->sub_meshes[0].mat->color.r);
			ImGui::Image(e->renderer->mesh->sub_meshes[0].mat->tex->id, ImVec2(50, 50));
		}
	}

	if (e->body) {
		if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) {
			vec3s scale = editor->physics->get_transformed_scale(editor->physics, e->body);
			ImGui::DragFloat3("transformed scale", &scale.x);
		}
	}

	inspector_draw_add_component(editor, e);
}

void draw_tree_node(struct editor *editor, struct entity *entity)
{
	float cursor_x = ImGui::GetCursorPosX();

	while (entity) {
		bool selected = entity == editor->selected_entity ? true : false;
		ImGui::PushID(entity->id);
		if (ImGui::Selectable(entity->name, selected)) {
			editor->selected_entity = entity;
		}

		if (entity->children) {
			ImGui::SetCursorPosX(cursor_x + 10.0f);
			draw_tree_node(editor, entity->children);
			ImGui::SetCursorPosX(cursor_x);
		}

		ImGui::PopID();
		entity = entity->next;
	}
}

void draw_hierarchy(struct editor *editor)
{
	ImGui::Begin("Hierarchy");

	for (int i = 0; i < editor->scene->entities.count; i++) {
		struct entity *e = &editor->scene->entities.data[i];
		if (!e->parent) {
			draw_tree_node(editor, e);
		}
	}

	if (editor->input->actions[LCTRL].state != CANCELED) {
		if (editor->input->actions[D].state == STARTED) {
			if (editor->selected_entity)
				editor->game->entity_duplicate(editor->scene, editor->selected_entity);
		}
	}

	if (editor->input->actions[DEL].state == STARTED && editor->selected_entity) {
		editor->game->destroy_entity(editor->scene, editor->selected_entity);
	}

	ImGui::End();
}

void draw_inspector(struct editor *editor)
{
	ImGui::Begin("inspector");
	if (editor->selected_entity) {
		inspector_draw_entity(editor, editor->selected_entity);
	}
	ImGui::End();
}

void update_editor_cam(struct editor *editor)
{
	struct input *input = editor->input;
	struct window *win = editor->win;
	struct scene *scene = editor->scene;

	if (input->actions[M1].state == STARTED) {
		input->lock_mouse(win->sdl_win, true);
	} else if (input->actions[M1].state == CANCELED) {
		input->lock_mouse(win->sdl_win, false);
	}

	if (input->actions[M1].state != CANCELED) {
		versors current_rot = editor->editor_cam->transform->rot;

		scene->pitch -= input->actions[MOUSE_DELTA].composite.y * scene->look_sens;
		scene->yaw -= input->actions[MOUSE_DELTA].composite.x * scene->look_sens;

		versors target_rot = glms_euler_zyx_quat((vec3s){ scene->pitch, scene->yaw, 0.0f });
		editor->game->set_rotation(editor->editor_cam->transform, target_rot);

		vec3s forward = editor->game->get_forward(editor->editor_cam->transform);
		vec3s right = editor->game->get_right(editor->editor_cam->transform);

		vec3s move_dir = glms_vec3_add(glms_vec3_scale(forward, input->actions[WASD].composite.y),
					       glms_vec3_scale(right, -input->actions[WASD].composite.x));

		vec3s new_pos = glms_vec3_add(editor->editor_cam->transform->pos,
					      glms_vec3_scale(move_dir, scene->move_speed * scene->dt));
		editor->game->set_position(editor->editor_cam->transform, new_pos);
	}
}

void update_editor(struct editor *editor)
{
	if (editor->mode == DEFAULT) {
		update_editor_cam(editor);
		editor->game->update_cameras(&editor->cameras);
	}
	ImGui::SetCurrentContext((ImGuiContext *)editor->imgui_ctx);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
	bool show = true;
	ImGui::ShowDemoWindow(&show);
	scene_view_draw(editor);
	draw_hierarchy(editor);
	draw_inspector(editor);
	debug_draw(editor);
	ImGui::Render();

	ImDrawData *draw_data = ImGui::GetDrawData();
	if (!draw_data)
		printf("INVALID DRAW DATA\n");

	ImGui_ImplOpenGL3_RenderDrawData(draw_data);
	ImGui::EndFrame();
	ImGui::UpdatePlatformWindows();
}

void process_event(SDL_Event *event)
{
	ImGui_ImplSDL3_ProcessEvent(event);
}

void init_editor(struct window *win, struct editor *editor)
{
	editor_arena = get_new_arena((size_t)1 << 30);
	editor->next_id = 1;
	editor->mode = DEFAULT;

	editor->entities.data = alloc_struct(&editor_arena, struct entity, 4096);
	editor->cameras.data = alloc_struct(&editor_arena, struct camera, 32);
	editor->transforms.data = alloc_struct(&editor_arena, struct transform, 4096);
	editor->mesh_renderers.data = alloc_struct(&editor_arena, struct mesh_renderer, 4096);

	editor->editor_cam = editor->game->get_new_entity(&editor->entities, &editor->transforms, &editor->next_id);
	editor->game->add_camera(&editor->cameras, editor->editor_cam);

	editor->show_demo = true;
	IMGUI_CHECKVERSION();
	editor->imgui_ctx = ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
	ImGui::StyleColorsDark();
	ImGui_ImplSDL3_InitForOpenGL(win->sdl_win, win->ctx);
	ImGui_ImplOpenGL3_Init("#version 460");

	ImGuiStyle style = ImGui::GetStyle();
	style.Alpha = 1.00f;
	style.DisabledAlpha = 0.60f;
	style.WindowPadding = ImVec2(7.00f, 8.00f);
	style.WindowRounding = 0.00f;
	style.WindowBorderSize = 0.00f;
	style.WindowMinSize = ImVec2(32.00f, 32.00f);
	style.WindowTitleAlign = ImVec2(0.00f, 0.50f);
	style.ChildRounding = 0.00f;
	style.ChildBorderSize = 0.00f;
	style.PopupRounding = 0.00f;
	style.PopupBorderSize = 0.00f;
	style.FramePadding = ImVec2(4.00f, 3.00f);
	style.FrameRounding = 0.00f;
	style.FrameBorderSize = 1.00f;
	style.ItemSpacing = ImVec2(4.00f, 4.00f);
	style.ItemInnerSpacing = ImVec2(3.00f, 4.00f);
	style.IndentSpacing = 21.00f;
	style.CellPadding = ImVec2(6.00f, 1.00f);
	style.ScrollbarSize = 9.00f;
	style.ScrollbarRounding = 0.00f;
	style.GrabMinSize = 18.00f;
	style.GrabRounding = 0.00f;
	style.TabRounding = 0.15f;
	style.TabBorderSize = 0.00f;
	style.TabCloseButtonMinWidthSelected = 18.00f;
	style.TabCloseButtonMinWidthUnselected = 1.00f;
	style.DisplayWindowPadding = ImVec2(0.00f, 14.00f);
	style.DisplaySafeAreaPadding = ImVec2(7.00f, 3.00f);
	style.MouseCursorScale = 1.00f;
	style.AntiAliasedLines = true;
	style.AntiAliasedLinesUseTex = true;
	style.AntiAliasedFill = true;
	style.CurveTessellationTol = 1.25f;
	style.CircleTessellationMaxError = 0.30f;

	ImVec4 *colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.66f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.56f, 1.00f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.11f, 0.61f, 0.97f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.27f, 0.80f, 0.98f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 0.39f, 1.00f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.77f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.77f, 0.77f, 0.77f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.23f, 0.48f, 1.00f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.41f, 0.46f, 1.00f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.19f, 0.39f, 0.80f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.56f, 0.98f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.13f, 0.66f, 1.00f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_InputTextCursor] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.17f, 0.59f, 0.99f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TabSelected] = ImVec4(0.28f, 0.67f, 1.00f, 1.00f);
	colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_TabDimmed] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
	colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(1.00f, 0.24f, 0.24f, 1.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_TreeLines] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void editor_reload(struct editor *editor)
{
}

extern "C" PETE_API void load_editor_functions(struct editor *editor)
{
	editor->init_editor = init_editor;
	editor->update_editor = update_editor;
	editor->process_event = process_event;
}
