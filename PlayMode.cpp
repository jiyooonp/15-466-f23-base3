#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

void fitInPlane(glm::vec3 &position)
{
	const double minX = -2.5;
	const double minY = -1;
	const double maxX = 2.5;
	const double maxY = 1;

	if (position.x > maxX)
		position.x = maxX;
	else if (position.x < minX)
		position.x = minX;
	if (position.y > maxY)
		position.y = maxY;
	else if (position.y < minY)
		position.y = minY;
}

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const *
								{
	MeshBuffer const *ret = new MeshBuffer(data_path("candyman.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> hexapod_scene(LoadTagDefault, []() -> Scene const *
						  { return new Scene(data_path("candyman.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
											 {
												 Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

												 scene.drawables.emplace_back(transform);
												 Scene::Drawable &drawable = scene.drawables.back();

												 drawable.pipeline = lit_color_texture_program_pipeline;

												 drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
												 drawable.pipeline.type = mesh.type;
												 drawable.pipeline.start = mesh.start;
												 drawable.pipeline.count = mesh.count; }); });

Load<Sound::Sample> good_object_sample(LoadTagDefault, []() -> Sound::Sample const *
									   { return new Sound::Sample(data_path("good.wav")); });
Load<Sound::Sample> bad_object_sample(LoadTagDefault, []() -> Sound::Sample const *
									  { return new Sound::Sample(data_path("bad.wav")); });

PlayMode::PlayMode() : scene(*hexapod_scene)
{
	// get pointers to leg for convenience:
	for (auto &transform : scene.transforms)
	{
		std::cout << transform.name << std::endl;
		if (transform.name == "Tobby")
			tobby = &transform;
		else if (transform.name == "Plane")
			plane = &transform;
		else if (transform.name == "Light")
			light = &transform;
		else if (transform.name == "good")
			good_object = &transform;
		else if (transform.name == "bad")
			bad_object = &transform;
	}
	if (tobby == nullptr)
		throw std::runtime_error("Tobby not found.");
	if (plane == nullptr)
		throw std::runtime_error("Plane not found.");
	if (light == nullptr)
		throw std::runtime_error("Light not found.");
	if (good_object == nullptr)
		throw std::runtime_error("Good Object not found.");
	if (bad_object == nullptr)
		throw std::runtime_error("Bad Object not found.");

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// start music loop playing:
	//  (note: position will be over-ridden in update())
	good_object_loop = Sound::loop_3D(*good_object_sample, 0.0f, get_good_object_position(), 10.0f);
	bad_object_loop = Sound::loop_3D(*bad_object_sample, 0.0f, get_bad_object_position(), 10.0f);
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_KEYDOWN)
	{
		if (evt.key.keysym.sym == SDLK_ESCAPE)
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_a)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s)
		{
			down.downs += 1;
			down.pressed = true;
			return true;
		}

		// move tobby
		else if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP)
	{
		if (evt.key.keysym.sym == SDLK_a)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s)
		{
			down.pressed = false;
			return true;
		}
		// move tobby

		else if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN)
	{
		if (SDL_GetRelativeMouseMode() == SDL_FALSE)
		{
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEMOTION)
	{
		if (SDL_GetRelativeMouseMode() == SDL_TRUE)
		{
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y));
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation * glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f)));
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{

	// move sound to follow leg tip position:
	good_object_loop->set_position(get_good_object_position(), 1.0f / 60.0f);
	bad_object_loop->set_position(get_bad_object_position(), 1.0f / 60.0f);

	// move tobby
	{
		// combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed)
			tobby->position.x -= tobby_speed * elapsed;
		if (!left.pressed && right.pressed)
			tobby->position.x += tobby_speed * elapsed;
		if (down.pressed && !up.pressed)
			tobby->position.y -= tobby_speed * elapsed;
		if (!down.pressed && up.pressed)
			tobby->position.y += tobby_speed * elapsed;
		fitInPlane(tobby->position);
	}

	{ // update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	// reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// set up light type and position for lit_color_texture_program:
	//  TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ // use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
						glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
						glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_good_object_position()
{
	// the vertex position here was read from the model in blender:
	return good_object->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}

glm::vec3 PlayMode::get_bad_object_position()
{
	// the vertex position here was read from the model in blender:
	return bad_object->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}