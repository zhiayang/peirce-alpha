// edit.cpp
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
	const char* get_prop_name();
	size_t get_prop_capacity();

	Styler disabled_style(bool disabled);
	Styler toggle_enabled_style(bool enabled);

	bool is_mouse_in_bounds();
	zbuf::str_view insert_prop_button(int shortcut_num, bool enabled);

	void editing_tools(Graph* graph)
	{
		imgui::PushID("__scope_editing");

		auto& sel = selection();

		imgui::Text("editing"); imgui::Dummy(lx::vec2(4));
		imgui::Indent();

		{
			auto s = disabled_style(selection().empty());
			auto ss = flash_style(SB_BUTTON_E_DELETE);
			if(imgui::Button("  \uf55a delete "))
			{
				auto items = sel.get();
				for(auto i : items)
					alpha::eraseItemFromParent(graph, i);
			}
		}

		{
			bool en = sel.count() == 1 || (sel.empty() && is_mouse_in_bounds());
			if(auto prop = insert_prop_button(1, en); !prop.empty() && en)
				alpha::insert(graph, sel[0], Item::var(prop));
		}

		{
			bool en = sel.count() == 1 || (sel.empty() && is_mouse_in_bounds());
			auto s = disabled_style(!en);
			auto ss = flash_style(SB_BUTTON_E_ADD_BOX);
			if(imgui::Button("2 \uf0fe add cut "))
				alpha::insertEmptyBox(graph, sel[0], lx::vec2(0, 0));
		}

		{
			bool en = !sel.empty() && sel.allSiblings();
			auto s = disabled_style(!en);
			auto ss = flash_style(SB_BUTTON_E_SURROUND);
			if(imgui::Button("3 \uf853 surround cut ")) // or \uf247
				alpha::surround(graph, sel);
		}

		imgui::Unindent();

		if(auto clips = ui::getClipboard().size(); clips > 0)
		{
			auto geom = geometry::get();

			auto curs = imgui::GetCursorPos();
			imgui::SetCursorPos(lx::vec2(curs.x, geom.sidebar.size.y - 34));
			imgui::TextUnformatted(zpr::sprint("{} item{} copied", clips, clips == 1 ? "" : "s").c_str());

			imgui::SetCursorPos(curs);
		}

		imgui::PopID();
	}


}
