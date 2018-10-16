#include "precompiled.h"

// based on https://www.shadertoy.com/view/MtSSRz

#include "util.h"
#include "stuff.h"
#include "shade.h"
#include "gpgpu.h"
#include "stefanfw.h"
int wsx = 1280, wsy = 720;
int scale = 1;
int sx = wsx / ::scale;
int sy = wsy / ::scale;

bool pause;

struct Walker {
	vec2 pos;
	vec2 vel;
	vec2 acc;
	Walker() {
		pos.x = randFloat(0, sx);
		pos.y = randFloat(0, sy);
	}
	void update() {
		if (app::getElapsedFrames() % 100 == 0) {
			acc = ci::randVec2() * ci::randFloat();
		}
		vel += acc;
		pos += vel;
	}
};

vector<Walker> walkers;

struct SApp : App {
	void setup()
	{
		createConsole();
		enableDenormalFlushToZero();
		disableGLReadClamp();

		setWindowSize(wsx, wsy);

		stefanfw::eventHandler.subscribeToEvents(*this);

		for (int i = 0; i < 1000; i++) {
			walkers.push_back(Walker());
		}
	}
	void update()
	{
		stefanfw::beginFrame();
		stefanUpdate();
		stefanDraw();
		stefanfw::endFrame();
	}

	void keyDown(KeyEvent e)
	{
		if(keys['p'] || keys['2'])
		{
			pause = !pause;
		}
	}

	void stefanUpdate() {
		for (auto& walker: walkers) {
			walker.update();
		}

		if(pause)
			std::this_thread::sleep_for(50ms);
	}
	void stefanDraw() {
		//static auto tex = maketex(sx, sy, GL_R16F);
		//gl::draw(tex, getWindowBounds());
		gl::clear();
		gl::begin(GL_POINTS);
		gl::color(Color::white());
		for (auto& walker : walkers) {
			gl::vertex(walker.pos);
		}
	}
};

CINDER_APP(SApp, RendererGl)