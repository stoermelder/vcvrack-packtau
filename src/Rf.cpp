#include "plugin.hpp"
#include <queue>

namespace StoermelderPackTau {
namespace Rf {

struct RfModule : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		COLLAPSED_LIGHT,
		NUM_LIGHTS
	};

	/** [Stored to JSON] */
	bool collapsed = false;
	/** [Stored to JSON] */
	std::list<std::tuple<ModuleWidget*, int, Vec, Vec>> modulePos;
	/** [Stored to JSON] */
	std::list<std::tuple<PortWidget*, Vec>> portPos;

	std::list<std::tuple<int, int, Vec>> modulePosTemp;

	dsp::SchmittTrigger modeTrigger;
	dsp::ClockDivider lightDivider;

	RfModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		lightDivider.setDivision(1024);
	}

	void process(const ProcessArgs& args) override {
		// Set channel lights infrequently
		if (lightDivider.process()) {
			lights[COLLAPSED_LIGHT].setBrightness(collapsed);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "collapsed", json_boolean(collapsed));

		json_t* modulePosJ = json_array();
		for (auto t : modulePos) {
			ModuleWidget* mw = std::get<0>(t);
			int i = std::get<1>(t);
			math::Vec pos = std::get<2>(t);

			json_t* modulePos1J = json_object();
			json_object_set_new(modulePos1J, "moduleId", json_integer(mw->module->id));
			json_object_set_new(modulePos1J, "i", json_integer(i));

			pos = pos.div(RACK_GRID_SIZE).round();
			json_t* posJ = json_pack("[i, i]", (int)pos.x, (int)pos.y);
			json_object_set_new(modulePos1J, "pos", posJ);

			json_array_append_new(modulePosJ, modulePos1J);
		}
		json_object_set_new(rootJ, "modulePos", modulePosJ);
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		collapsed = json_boolean_value(json_object_get(rootJ, "collapsed"));

		json_t* modulePosJ = json_object_get(rootJ, "modulePos");
		size_t modulePosIdx;
		json_t* modulePos1J;
		json_array_foreach(modulePosJ, modulePosIdx, modulePos1J) {
			int moduleId = json_integer_value(json_object_get(modulePos1J, "moduleId"));
			int i = json_integer_value(json_object_get(modulePos1J, "i"));

			json_t* posJ = json_object_get(modulePos1J, "pos");
			double x, y;
			json_unpack(posJ, "[F, F]", &x, &y);
			math::Vec pos = math::Vec(x, y);
			pos = pos.mult(RACK_GRID_SIZE);

			modulePosTemp.push_back(std::make_tuple(moduleId, i, pos));
		}
	}
};


struct UnmappableTL1105 : OpaqueWidget {
	widget::FramebufferWidget* fb;
	CircularShadow* shadow;
	widget::SvgWidget* sw;
	std::vector<std::shared_ptr<Svg>> frames;

	UnmappableTL1105() {
		fb = new widget::FramebufferWidget;
		addChild(fb);

		shadow = new CircularShadow;
		fb->addChild(shadow);
		shadow->box.size = math::Vec();

		sw = new widget::SvgWidget;
		fb->addChild(sw);

		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/TL1105_0.svg")));
		addFrame(APP->window->loadSvg(asset::system("res/ComponentLibrary/TL1105_1.svg")));
	}

	void addFrame(std::shared_ptr<Svg> svg)	{
		frames.push_back(svg);
		// If this is our first frame, automatically set SVG and size
		if (!sw->svg) {
			sw->setSvg(svg);
			box.size = sw->box.size;
			fb->box.size = sw->box.size;
			// Move shadow downward by 10%
			shadow->box.size = sw->box.size;
			shadow->box.pos = math::Vec(0, sw->box.size.y * 0.10);
		}
	}

	void onDragStart(const event::DragStart& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;
		sw->setSvg(frames[1]);
		fb->dirty = true;
		doAction();
	}

	void onDragEnd(const event::DragEnd& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT) return;
		sw->setSvg(frames[0]);
		fb->dirty = true;
	}

	virtual void doAction() { }
};



struct RfWidget : ModuleWidget {
	RfModule* module;
	bool firstRun = true;
	Vec oldPos;

	RfWidget(RfModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Rf.svg")));
		this->module = module;

		struct CollapseButton : UnmappableTL1105 {
			RfWidget* mw;
			void doAction() override {
				if (mw->isCollapsed())
					mw->expand();
				else
					mw->collapse();
			}
		};

		addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(15.f, 259.9f), module, RfModule::COLLAPSED_LIGHT));

		auto collapseButton = createWidgetCentered<CollapseButton>(Vec(15.f, 275.2f));
		collapseButton->mw = this;
		addChild(collapseButton);
	}

	bool isCollapsed() {
		return module->collapsed;
	}

	void expand() {
		for (std::tuple<ModuleWidget*, int, Vec, Vec> t : module->modulePos) {
			ModuleWidget* mw = std::get<0>(t);
			
			Vec oldPos = mw->box.pos;
			mw->box.pos = std::get<2>(t).plus(box.pos);
			mw->box.size = std::get<3>(t);
			mw->visible = true;

			RfWidget* smw = dynamic_cast<RfWidget*>(mw);
			if (smw) {	
				smw->updatePortPos(oldPos.minus(mw->box.pos).plus(mw->box.pos.minus(box.pos)));
			}
		}

		module->modulePos.clear();

		for (std::tuple<PortWidget*, Vec> t : module->portPos) {
			PortWidget* pw = std::get<0>(t);
			pw->box.pos = std::get<1>(t);
		}
		module->portPos.clear();

		// Does nothing but fixes https://github.com/VCVRack/Rack/issues/1444
		APP->scene->rack->requestModulePos(this, this->box.pos);

		module->collapsed = false;
	}

	void collapse() {
		/* if (module->mode == MODE::LEFTRIGHT || module->mode == MODE::RIGHT) */ {
			Module* m = module;
			int i = 0;
			while (true) {
				if (!m || m->rightExpander.moduleId < 0) break;
				m = m->rightExpander.module;
				i++;
				ModuleWidget* mw = APP->scene->rack->getModule(m->id);
				module->modulePos.push_back(std::make_tuple(mw, i, mw->box.pos.minus(box.pos), mw->box.size));
				collapseModule(mw, i);
			}
		}
		/* if (module->mode == MODE::LEFTRIGHT || module->mode == MODE::LEFT) */ {
			Module* m = module;
			int i = 0;
			while (true) {
				if (!m || m->leftExpander.moduleId < 0) break;
				m = m->leftExpander.module;
				i--;
				ModuleWidget* mw = APP->scene->rack->getModule(m->id);
				module->modulePos.push_back(std::make_tuple(mw, i, mw->box.pos.minus(box.pos), mw->box.size));
				collapseModule(mw, i);
			}
		}

		// Does nothing but fixes https://github.com/VCVRack/Rack/issues/1444
		APP->scene->rack->requestModulePos(this, this->box.pos);

		module->collapsed = true;
	}

	void collapseModule(ModuleWidget* mw, int i) {
		int o = 536870; // a random magic number, be aware
		Vec p = Vec(o / (int)RACK_GRID_WIDTH, o / (int)RACK_GRID_HEIGHT).plus(Vec(i, module->id));
		
		Vec oldPos = mw->box.pos;
		mw->visible = false;
		mw->box.pos = p.round().mult(RACK_GRID_SIZE);
		mw->box.size = Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		collapsePorts(mw);

		RfWidget* smw = dynamic_cast<RfWidget*>(mw);
		if (smw) smw->updatePortPos(oldPos.minus(mw->box.pos).minus(oldPos.minus(box.pos)));
	}

	void collapsePorts(ModuleWidget* mw) {
		std::queue<Widget*> q;
		q.push(mw);

		while (q.size() > 0) {
			Widget* w = q.front();
			q.pop();
			for (Widget* c : w->children) {
				PortWidget* pw = dynamic_cast<PortWidget*>(c);
				if (pw) {
					module->portPos.push_back(std::make_tuple(pw, pw->box.pos));

					Vec pos = box.pos.minus(mw->box.pos).plus(Vec(15.f - 11.f, 306.7f - 11.f));
					pos = pw->box.pos.minus(pw->getRelativeOffset(Vec(), mw)).plus(pos);
					pw->box.pos = pos;
				}
				else {
					q.push(c);
				}
			}
		}
	}

	void load() {
		if (module->modulePosTemp.size() > 0) {
			for (std::tuple<int, int, Vec> t : module->modulePosTemp) {
				int moduleId = std::get<0>(t);
				int i = std::get<1>(t);
				Vec pos = std::get<2>(t);

				ModuleWidget* mw = APP->scene->rack->getModule(moduleId);
				module->modulePos.push_back(std::make_tuple(mw, i, pos, mw->box.size));
				collapseModule(mw, i);
			}
			module->modulePosTemp.clear();
		}
	}

	void step() override {
		ModuleWidget::step();
		if (!module) return; 

		if (firstRun) {
			load();

			// Does nothing but fixes https://github.com/VCVRack/Rack/issues/1444
			APP->scene->rack->requestModulePos(this, this->box.pos);

			firstRun = false;
			oldPos = box.pos;
		}

		if (!box.pos.isEqual(oldPos)) {
			Vec delta = box.pos.minus(oldPos);
			updatePortPos(delta);
			oldPos = box.pos;
		}
	}

	void updatePortPos(Vec delta) {
		for (std::tuple<PortWidget*, Vec> t : module->portPos) {
			PortWidget* pw = std::get<0>(t);
			pw->box.pos = pw->box.pos.plus(delta);
		}

		for (std::tuple<ModuleWidget*, int, Vec, Vec> t : module->modulePos) {
			RfWidget* mw = dynamic_cast<RfWidget*>(std::get<0>(t));
			if (mw) mw->updatePortPos(delta);
		}
	}
};

} // namespace Rf
} // namespace StoermelderPackTau

Model* modelRf = createModel<StoermelderPackTau::Rf::RfModule, StoermelderPackTau::Rf::RfWidget>("Rf");