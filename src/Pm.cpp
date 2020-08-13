#include "plugin.hpp"
#include "settings.hpp"

namespace StoermelderPackTau {
namespace Pm {

struct PmModule : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_TRIG,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		LIGHT_ACTIVE,
		NUM_LIGHTS
	};

	PmModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}
};

struct PmContainer : widget::Widget {
	struct ChannelsItem : MenuItem {
		PortWidget* pw;
		void step() override {
			rightText = string::f("%d", pw->module->outputs[pw->portId].getChannels());
			MenuItem::step();
		}
	};

	struct DisconnectItem : MenuItem {
		PortWidget* pw;
		void onAction(const event::Action& e) override {
			CableWidget* cw = APP->scene->rack->getTopCable(pw);
			if (cw) {
				// history::CableRemove
				history::CableRemove* h = new history::CableRemove;
				h->setCable(cw);
				APP->history->push(h);

				APP->scene->rack->removeCable(cw);
				delete cw;
			}
		}
	};

	struct DisconnectAllItem : MenuItem {
		PortWidget* pw;
		void onAction(const event::Action& e) override {
			std::list<CableWidget*> c = APP->scene->rack->getCablesOnPort(pw);

			history::ComplexAction* h = new history::ComplexAction;
			h->name = "remove all cables";
			for (CableWidget* cw : c) {
				history::CableRemove* ch = new history::CableRemove;
				ch->setCable(cw);
				h->push(ch);

				APP->scene->rack->removeCable(cw);
				delete cw;
			}
			APP->history->push(h);
		}
	};

	struct NextColorItem : MenuItem {
		PortWidget* pw;
		void onAction(const event::Action& e) override {
			CableWidget* cw = APP->scene->rack->getTopCable(pw);
			int cid = APP->scene->rack->nextCableColorId++;
			APP->scene->rack->nextCableColorId %= settings::cableColors.size();
			cw->color = settings::cableColors[cid];
		}
	};

	struct RotateItem : MenuItem {
		PortWidget* pw;
		void onAction(const event::Action& e) override {
			Widget* cc = APP->scene->rack->cableContainer;
			std::list<Widget*>::iterator it;
			for (it = cc->children.begin(); it != cc->children.end(); it++) {
				CableWidget* cw = dynamic_cast<CableWidget*>(*it);
				assert(cw);
				// Ignore incomplete cables
				if (!cw->isComplete())
					continue;
				if (cw->inputPort == pw || cw->outputPort == pw)
					break;
			}
			if (it != cc->children.end()) {
				cc->children.splice(cc->children.end(), cc->children, it);
			}
		}
	};

	/*
	struct DisconnectCableSubMenuItem : MenuItem {
		struct DisconnectCableMenuItem : MenuItem {
		};

		PortWidget *port;
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			for (CableWidget *cw : APP->scene->rack->getCablesOnPort(port)) {
				if (!cw->isComplete())
					continue;

				std::string n = "to module " + cw->inputPort->module->model->name;
				menu->addChild(construct<DisconnectCableMenuItem>(&MenuItem::text, n));
			}
			return menu;
		}
	};
	*/

	struct GotoInputItem : MenuItem {
		PortWidget* pw;
		void onAction(const event::Action& e) override {
			CableWidget* cw = APP->scene->rack->getTopCable(pw);
			Vec source = cw->outputPort->getRelativeOffset(Vec(0, 0), APP->scene->rack);
			Vec target = cw->inputPort->getRelativeOffset(Vec(0, 0), APP->scene->rack);
			target = target.minus(source);
			target = target.mult(APP->scene->rackScroll->zoomWidget->zoom);
			APP->scene->rackScroll->offset = APP->scene->rackScroll->offset.plus(target);
		}
	};

	struct GotoOutputItem : MenuItem {
		PortWidget* pw;
		void onAction(const event::Action& e) override {
			CableWidget* cw = APP->scene->rack->getTopCable(pw);
			Vec source = cw->inputPort->getRelativeOffset(Vec(0, 0), APP->scene->rack);
			Vec target = cw->outputPort->getRelativeOffset(Vec(0, 0), APP->scene->rack);
			target = target.minus(source);
			target = target.mult(APP->scene->rackScroll->zoomWidget->zoom);
			APP->scene->rackScroll->offset = APP->scene->rackScroll->offset.plus(target);
		}
	};

	void onButton(const event::Button& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			Widget* w = APP->event->getHoveredWidget();
			if (!w) goto j;
			PortWidget* pw = dynamic_cast<PortWidget*>(w);
			if (!pw) goto j;
			int c = APP->scene->rack->getCablesOnPort(pw).size();
			if (c == 0) goto j;

			e.consume(this);
			Menu* menu = createMenu();
			if (pw->type == PortWidget::Type::OUTPUT) {
				menu->addChild(createMenuLabel("Output-port"));
				menu->addChild(construct<ChannelsItem>(&MenuItem::text, "Output channels", &ChannelsItem::pw, pw));
				if (c == 1) menu->addChild(construct<GotoInputItem>(&MenuItem::text, "Go to input-port", &GotoInputItem::pw, pw));
				menu->addChild(construct<DisconnectItem>(&MenuItem::text, "Disconnect", &DisconnectItem::pw, pw));
				if (c > 1) menu->addChild(construct<DisconnectAllItem>(&MenuItem::text, "Disconnect all", &DisconnectAllItem::pw, pw));
				if (c > 1) menu->addChild(construct<RotateItem>(&MenuItem::text, "Rotate ordering", &RotateItem::pw, pw));
				menu->addChild(construct<NextColorItem>(&MenuItem::text, "Next color", &NextColorItem::pw, pw));
			}
			else {
				menu->addChild(createMenuLabel("Input-port"));
				menu->addChild(construct<GotoOutputItem>(&MenuItem::text, "Go to output-port", &GotoOutputItem::pw, pw));
				menu->addChild(construct<DisconnectItem>(&MenuItem::text, "Disconnect", &DisconnectItem::pw, pw));
				menu->addChild(construct<NextColorItem>(&MenuItem::text, "Next color", &NextColorItem::pw, pw));
			}
		}

		j:
		Widget::onButton(e);
	}
};

struct PmWidget : ModuleWidget {
	PmContainer* pmContainer = NULL;
	bool active = false;

	PmWidget(PmModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Pm.svg")));

		addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(15.f, 330.0f), module, PmModule::LIGHT_ACTIVE));

		if (module) {
			active = registerSingleton("Pm", this);
			if (active) {
				pmContainer = new PmContainer;
				// This is where the magic happens: add a new widget on top-level to Rack
				APP->scene->rack->addChild(pmContainer);
			}
		}
	}

	~PmWidget() {
		if (pmContainer && active) {
			unregisterSingleton("Pm", this);
			APP->scene->rack->removeChild(pmContainer);
			delete pmContainer;
		}
	}

	void step() override {
		if (pmContainer) {
			module->lights[PmModule::LIGHT_ACTIVE].setBrightness(active);
		}
		ModuleWidget::step();
	}
};

} // namespace Pm
} // namespace StoermelderPackTau

Model* modelPm = createModel<StoermelderPackTau::Pm::PmModule, StoermelderPackTau::Pm::PmWidget>("Pm");