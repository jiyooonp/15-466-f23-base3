#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

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
									   { return new Sound::Sample(data_path("good_2.wav")); });
Load<Sound::Sample> bad_object_sample(LoadTagDefault, []() -> Sound::Sample const *
									  { return new Sound::Sample(data_path("bad_2.wav")); });

PlayMode::PlayMode() : scene(*hexapod_scene)
{
	// get pointers to leg for convenience:
	for (auto &transform : scene.transforms)
	{
		if (transform.name == "Tobby")
			tobby = &transform;
		else if (transform.name == "Plane")
			plane = &transform;
		else if (transform.name == "Light")
			light = &transform;
		else if (transform.name == "Good")
			good_object = &transform;
		else if (transform.name == "Bad")
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
	good_object_loop = Sound::loop_3D(*good_object_sample, 0.5f, get_good_object_position(), 1.0f);
	// bad_object_loop = Sound::loop_3D(*bad_object_sample, 0.0f, get_bad_object_position(), 10.0f);
	level_up(false);
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_KEYDOWN)
	{
		if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.downs += 1;
			space.pressed = true;
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
		if (evt.key.keysym.sym == SDLK_LEFT)
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
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			space.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	if (lives <= 0)
	{
		game_over();
		return;
	}
	// move sound to follow leg tip position:
	good_object_loop->set_position(get_good_object_position(), 1.0f / 60.0f);
	// bad_object_loop->set_position(get_bad_object_position(), 1.0f / 60.0f);

	// move tobby
	{
		// combine inputs into a move:
		if (left.pressed && !right.pressed)
			tobby->position.x -= tobby_speed * elapsed;
		if (!left.pressed && right.pressed)
			tobby->position.x += tobby_speed * elapsed;
		if (down.pressed && !up.pressed)
			tobby->position.y -= tobby_speed * elapsed;
		if (!down.pressed && up.pressed)
			tobby->position.y += tobby_speed * elapsed;
		if (space.pressed)
			tobby->position.z += tobby_speed * elapsed;
		if (!space.pressed)
			tobby->position.z -= tobby_speed * elapsed;
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

	{ // make good object fly
		good_object->position.x += object_vector[0] * elapsed * level;
		good_object->position.y += object_vector[1] * elapsed * level;
		good_object->position.z += object_vector[2] * elapsed * level;
	}

	// check for collisions
	if (checkCollisionTobbyObject(tobby, good_object))
	{
		// levels up
		level += 1;
		level_up(true);
		std::cout << "good object collision" << std::endl;
	}
	else
	{
		check_object_in_frame(good_object->position);
	}
	if (checkCollisionTobbyObject(tobby, bad_object))
	{
		// std::cout << "bad object collision" << std::endl;
	}
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
		std::string score_str = "Score: " + std::to_string(score);
		std::string level_str = "Level: " + std::to_string(level);
		std::string lives_str = "Lives: " + std::to_string(lives);

		std::string display_str = score_str + " | " + level_str + " | " + lives_str;
		if (is_game_over)
			display_str = "Game Over | " + score_str;
		lines.draw_text(display_str,
						glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(display_str,
						glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0xff));
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

void PlayMode::level_up(bool add_score)
{
	if (add_score)
		score += 1;
	// move good object to a random position:
	std::mt19937 mt;
	mt.seed(std::random_device()());
	std::uniform_real_distribution<float> init_dist_x(-2.1f, 2.0f);
	std::uniform_real_distribution<float> init_dist_z(-4.0f, 4.0f);

	object_init_position[0] = init_dist_x(mt);
	object_init_position[1] = -16.0f;
	object_init_position[2] = init_dist_z(mt);

	good_object->position.x = object_init_position[0];
	good_object->position.y = object_init_position[1];
	good_object->position.z = object_init_position[2];

	std::uniform_real_distribution<float> object_speed_dist(0.1f, 1.0f);
	object_speed = object_speed_dist(mt);

	std::uniform_real_distribution<float> goal_dist_z(0.2f, 0.8f);
	std::uniform_real_distribution<float> goal_dist_y(-0.8f, 0.8f);
	object_goal_position[0] = object_init_position[0]; // goal_dist_x(mt);
	object_goal_position[1] = goal_dist_y(mt);
	object_goal_position[2] = goal_dist_z(mt);

	object_vector = {
		(object_goal_position[0] - object_init_position[0]),
		(object_goal_position[1] - object_init_position[1]),
		(object_goal_position[2] - object_init_position[2])};
	// normalize vector
	float length = sqrt(object_vector[0] * object_vector[0] + object_vector[1] * object_vector[1] + object_vector[2] * object_vector[2]);
	object_vector[0] /= length;
	object_vector[1] /= length;
	object_vector[2] /= length;

	// move bad object to a random position:
	// bad_object->position.x = object_goal_position[0];
	// bad_object->position.y = object_goal_position[1];
	// bad_object->position.z = object_goal_position[2];
}
void PlayMode::game_over()
{
	// game over
	// std::cout << "Game Over" << std::endl;
	is_game_over = true;
	good_object_loop->stop();
}
void PlayMode::check_object_in_frame(glm::vec3 &position)
{
	// out of the frame
	if (((position.x > 3.0f || position.x < -3.0f) && (position.y > 3)) || position.z < -5 || position.y > 3)
	{
		// lose life
		if (lives > 0)
		{
			lives -= 1;
			std::cout << "lost a life. Now: " << lives << std::endl;
			level_up(false);
		}
		else
		{
			game_over();
		}
	}
}

bool checkCollisionTobbyObject(Scene::Transform *tobby, Scene::Transform *object)
{
	glm::vec3 tobby_position = tobby->make_local_to_world() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	tobby_position.z += 0.5f;
	glm::vec3 object_position = object->make_local_to_world() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	float distance = glm::distance(tobby_position, object_position);
	if (distance < 0.2f)
		return true;
	else if (std::abs(tobby->position.z - object->position.z) < 1 && std::abs(tobby->position.x - object->position.x) < 0.3 && std::abs(tobby->position.y - object->position.y) < 0.2)
		return true;
	return false;
}
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
	if (position.z < 0)
		position.z = 0;
	else if (position.z > 1)
		position.z = 1;
}
