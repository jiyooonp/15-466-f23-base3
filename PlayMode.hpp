#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// hexapod leg to wobble:
	Scene::Transform *tobby = nullptr;
	Scene::Transform *plane = nullptr;
	Scene::Transform *light = nullptr;
	Scene::Transform *good_object = nullptr;
	Scene::Transform *bad_object = nullptr;

	glm::vec3 get_good_object_position();
	glm::vec3 get_bad_object_position();

	// music coming from the tip of the leg (as a demonstration):
	std::shared_ptr<Sound::PlayingSample> good_object_loop;
	std::shared_ptr<Sound::PlayingSample> bad_object_loop;

	// camera:
	Scene::Camera *camera = nullptr;

	// tobby
	float tobby_speed = 1.0f;

	// game state
	int score = 0;
	int lives = 3;
	float time = 0.0f;

	int level = 1;
};
