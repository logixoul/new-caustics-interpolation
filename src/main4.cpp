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
float noiseTimeDim = 0.0f;

bool pause;

void updateConfig() {
}

struct SApp : App {
	void setup()
	{
		createConsole();
		enableDenormalFlushToZero();
		disableGLReadClamp();

		setWindowSize(wsx, wsy);

		stefanfw::eventHandler.subscribeToEvents(*this);
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
		if(keys['r'])
		{
		}
		if(keys['p'] || keys['2'])
		{
			pause = !pause;
		}
	}
	float noiseProgressSpeed;

	void stefanUpdate() {
		noiseProgressSpeed=cfg1::getOpt("noiseProgressSpeed", .01f / 5.0f,
			[&]() { return keys['s']; },
			[&]() { return expRange(mouseY, 0.01f, 100.0f); });
		
		if(!pause) {
			noiseTimeDim += noiseProgressSpeed;
		}

		if(pause)
			std::this_thread::sleep_for(50ms);
	}
	void stefanDraw() {
		static Array2D<vec3> gradientMap = ci::Surface8u(ci::loadImage("gradientmap.png"));
		static auto gradientMapTex = gtex(gradientMap);

		globaldict["noiseTimeDim"] = noiseTimeDim;
		static auto tex = maketex(sx, sy, GL_R16F);
		tex = shade2(tex,
			"float nscale = 1;"
			"vec2 pf = tc * texSize / texSize.x;"
			"float f = 0;"
			"for(int i = 0; i < 5; i++) {"
			"	nscale *= 2.0f; f += abs(raw_noise_3d(pf.x * nscale, pf.y * nscale, noiseTimeDim)) / nscale;"
			"}"
			"f *= 2.0f;"
			"_out.r = f;"
			,
			ShadeOpts(),
			FileCache::get("simplexnoise3d.fs.glsl")
			);

		tex->setWrapS(GL_REPEAT);
		tex->setWrapT(GL_REPEAT);
		auto tex2 = shade2(tex, gradientMapTex,
		"float f = fetch1();"
		"f = fetch1(tex, tc + vec2(0.0, f*.2));"
		"f = 1.0f / f;"
		"f *= .6f;"
		"f = pow(f, 3.0f);"
		"f /= f + 1.0f;"
		"vec3 c = fetch3(tex2, vec2(f, 0.0));"
		"vec3 c2 = mix(c, vec3(1.3, .6, 0.0), .71);"
		"vec3 cHCL = RGB2HCL(c);"
		"vec3 c2HCL = RGB2HCL(c2);"
		"c2HCL.z = cHCL.z;"
		"c2 = HCL2RGB(c2HCL);"
		"c = mix(c2, c, tc.y);"
		//"c /= 1.0 + length(tc-vec3(.5))* 2.0;"
		"_out = c;",
		ShadeOpts().ifmt(GL_RGB8),
		FileCache::get("hcl_lib.fs")
		);
		gl::draw(tex2, getWindowBounds());
	}
};

CINDER_APP(SApp, RendererGl)