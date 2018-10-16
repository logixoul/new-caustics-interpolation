#include "precompiled.h"

// based on https://www.shadertoy.com/view/MtSSRz

#include "util.h"
#include "stuff.h"
#include "shade.h"
#include "gpgpu.h"
#include "stefanfw.h"
#include "convolve_fft.h"
#include "simplexnoise.h"
int wsx = 1280, wsy = 720;
int scale = 4;
int sx = wsx / ::scale;
int sy = wsy / ::scale;

float noiseTimeDim = 0;

bool pause;

gl::TextureRef tex;

struct Walker {
	vec2 pos;
	vec2 vel;
	vec2 acc;
	vec2 displacement;
	Walker() {
		pos.x = randFloat(0, sx);
		pos.y = randFloat(0, sy);
	}
	void update() {
		/*if (app::getElapsedFrames() % 100 == 0) {
			acc = ci::randVec2() * ci::randFloat() * .1f;
		}
		vel += acc;
		vel *= .8f;
		pos += vel;
		pos.x = fmod(pos.x, sx - 1);
		pos.y = fmod(pos.y, sy - 1);
		if (pos.x < 0) pos.x += sx - 1;
		if (pos.y < 0) pos.y += sy - 1;*/
		//vec2 displacement;
		float nscale = 10 / (float)sx; // both for x and y so we preserve aspect ratio
		displacement.x = raw_noise_4d(pos.x * nscale, pos.y * nscale, noiseTimeDim, 0.0) * 30.0f;
		displacement.y = raw_noise_4d(pos.x * nscale, pos.y * nscale, noiseTimeDim, 1.0) * 30.0f;
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
		noiseTimeDim += .008f;
		for (auto& walker: walkers) {
			walker.update();
		}

		Array2D<float> img(sx, sy, 0);
		for (auto& walker : walkers) {
			img.wr(walker.pos + walker.displacement) += 1.0f;
		}

		static auto invKernelf = getInvKernelf(img.Size(), [](float f) { return 1.0f / sq(f);  });
		static auto longTailKernelf = getInvKernelf(img.Size(), [](float f) { return 1.0f / (1 + f);  });

		auto convolved = convolveFft(img, longTailKernelf);

		auto punctured = convolved.clone();
		int count = 0;
		forxy(punctured) {
			punctured(p) *= img(p);
			if (punctured(p) > 0 && punctured(p) < 1)
				count++;
		}
		cout << "count = " << count << endl;

		auto puncturedConvolved = convolveFft(punctured, invKernelf);

		auto puncturedInterpolated = puncturedConvolved.clone();

		forxy(puncturedInterpolated) {
			puncturedInterpolated(p) /= convolved(p);
		}

		tex = gtex(puncturedInterpolated);

		if(pause)
			std::this_thread::sleep_for(50ms);
	}
	void stefanDraw() {
		//static auto tex = maketex(sx, sy, GL_R16F);
		//gl::draw(tex, getWindowBounds());
		/*gl::clear();
		gl::begin(GL_POINTS);
		gl::color(Color::white());
		for (auto& walker : walkers) {
			gl::vertex(walker.pos);
		}
		gl::end();*/
		auto tex2 = shade2(tex,
			"float f = fetch1() * 100.0f;"
			//"_out.r = f / (f + 1.0f);");
			"f = smoothstep(0, 1, f);"
			"f = smoothstep(0, 1, f);"
			"_out.r = f;");
		gl::clear();
		gl::setMatricesWindow(getWindowSize(), false);
		gl::draw(tex2, getWindowBounds());
	}
};

CINDER_APP(SApp, RendererGl)