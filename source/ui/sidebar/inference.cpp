// inference.cpp
// Copyright (c) 2021, zhiayang
// Licensed under the Apache License Version 2.0.

#include "ui.h"
#include "alpha.h"
#include "imgui/imgui.h"

namespace imgui = ImGui;
using namespace alpha;

namespace ui
{
	Styler flash_style(int button);
	char* get_prop_name();
	size_t get_prop_capacity();

	Styler disabled_style(bool disabled);
	Styler toggle_enabled_style(bool enabled);
	Styler disabled_style_but_dont_disable(bool disabled);

	zbuf::str_view insert_prop_button(int shortcut_num, bool enabled);

	static bool use_prop_name = false;
	bool insert_with_prop_name()
	{
		return use_prop_name;
	}

	void inferModeChanged(Graph* graph, bool active)
	{
		(void) graph;
		(void) active;
	}

	void perform_insertion(Graph* graph, Item* parent)
	{
		if(use_prop_name)
		{
			auto prop = zbuf::str_view(get_prop_name(), strlen(get_prop_name()));
			if(prop.length() == 0)
				return;

			alpha::insertAtOddDepth(graph, parent, Item::var(prop));
		}
		else
		{
			if(!alpha::haveIterationTarget(graph))
				return;

			auto thing = graph->iteration_target->clone();
			alpha::insertAtOddDepth(graph, parent, thing);
		}
	}


	void inference_tools(Graph* graph)
	{
		imgui::PushID("__scope_infer");

		auto geom = geometry::get();
		auto& theme = ui::theme();
		auto& sel = selection();

		imgui::Text("inference"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			{
				auto s = disabled_style(!alpha::canInsert(graph, get_prop_name(), insert_with_prop_name()));
				auto ss = flash_style(SB_BUTTON_INSERT);

				bool insert = imgui::Button("1 \uf055 insert ");
				imgui::Indent(14);
				{
					s.pop();
					ss.pop();

					{
						auto ss = disabled_style_but_dont_disable(use_prop_name);
						if(imgui::RadioButtonEx("iter target", !use_prop_name, 11))
							use_prop_name = false;
					}

					{
						auto ss = disabled_style_but_dont_disable(!use_prop_name);
						if(imgui::RadioButtonEx("fresh: ", use_prop_name, 11))
							use_prop_name = true;

						imgui::SameLine();

						auto s = Styler();
						imgui::SetNextItemWidth(64);
						s.push(ImGuiCol_FrameBg, theme.textFieldBg);
						imgui::InputTextWithHint("", " prop", get_prop_name(), get_prop_capacity());
					}
				}
				imgui::Unindent(14);

				if(insert && sel.count() == 1)
					perform_insertion(graph, sel[0]);
			}



			// insert anything into an odd depth
		#if 0
			if(auto prop = insert_prop_button(1, alpha::canInsert(graph, get_prop_name())); !prop.empty()
				|| alpha::haveIterationTarget(graph))
			{
				alpha::insertAtOddDepth(graph,
					/* parent: */ sel.empty() ? graph->iteration_target : sel[0],
					Item::var(prop));
			}
		#endif

			{
				// fa-minus-circle
				auto s = disabled_style(!alpha::canErase(graph));
				auto ss = flash_style(SB_BUTTON_ERASE);
				if(imgui::Button("2 \uf056 erase "))
					alpha::eraseFromEvenDepth(graph, sel[0]);
			}
		}
		imgui::Unindent();

		imgui::NewLine();

		imgui::Text("double cut"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			{
				auto s = disabled_style(!alpha::canInsertDoubleCut(graph));
				auto ss = flash_style(SB_BUTTON_DBL_ADD);
				if(imgui::Button("3 \uf5ff add "))
					alpha::insertDoubleCut(graph, sel);
			}

			{
				auto s = disabled_style(!alpha::canRemoveDoubleCut(graph));
				auto ss = flash_style(SB_BUTTON_DBL_DEL);
				if(imgui::Button("4 \uf5fe remove "))
					alpha::removeDoubleCut(graph, sel);
			}
		}
		imgui::Unindent();

		imgui::NewLine();

		imgui::Text("iteration"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();
		{
			{
				bool desel = (sel.count() == 0 && graph->iteration_target != nullptr);

				// crosshairs
				auto s = disabled_style(!alpha::canSelect(graph));
				auto ss = flash_style(SB_BUTTON_SELECT);
				if(imgui::Button(desel ? "5 \uf05b deselect " : "5 \uf05b select "))
					alpha::selectTargetForIteration(graph, sel.count() == 1 ? sel[0] : nullptr);
			}

			{
				// map-marker-plus
				auto s = disabled_style(!alpha::canIterate(graph));
				auto ss = flash_style(SB_BUTTON_ITER);
				if(imgui::Button("6 \uf60a iterate "))
					alpha::iterate(graph, sel[0]);
			}

			{
				// map-marker-minus
				auto s = disabled_style(!alpha::canDeiterate(graph));
				auto ss = flash_style(SB_BUTTON_DEITER);
				if(imgui::Button("7 \uf609 deiterate "))
					alpha::deiterate(graph, sel[0]);
			}
		}
		imgui::Unindent();


		if(alpha::haveIterationTarget(graph))
		{
			auto curs = imgui::GetCursorPos();
			imgui::SetCursorPos(lx::vec2(curs.x, geom.sidebar.size.y - 56));
			imgui::TextUnformatted("iteration target");
		}

		if(sel.count() == 1)
		{
			auto curs = imgui::GetCursorPos();
			imgui::SetCursorPos(lx::vec2(curs.x, geom.sidebar.size.y - 34));
			imgui::TextUnformatted(zpr::sprint("depth: {}", sel[0]->depth()).c_str());
		}

		imgui::PopID();
	}
}
