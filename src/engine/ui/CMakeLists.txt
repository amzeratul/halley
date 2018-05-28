project (halley-ui)

include_directories(${Boost_INCLUDE_DIR} "include" "include/halley/ui" "../core/include" "../utils/include" "../audio/include")

set(SOURCES
        "src/ui/ui_anchor.cpp"
        "src/ui/ui_data_bind.cpp"
        "src/ui/ui_event.cpp"
        "src/ui/ui_factory.cpp"
        "src/ui/ui_factory_tester.cpp"
        "src/ui/ui_input.cpp"
        "src/ui/ui_painter.cpp"
        "src/ui/ui_parent.cpp"
        "src/ui/ui_root.cpp"
        "src/ui/ui_sizer.cpp"
        "src/ui/ui_style.cpp"
        "src/ui/ui_stylesheet.cpp"
        "src/ui/ui_validator.cpp"
        "src/ui/ui_widget.cpp"

        "src/ui/widgets/ui_animation.cpp"
        "src/ui/widgets/ui_button.cpp"
        "src/ui/widgets/ui_checkbox.cpp"
        "src/ui/widgets/ui_clickable.cpp"
        "src/ui/widgets/ui_dropdown.cpp"
        "src/ui/widgets/ui_gamepad_image.cpp"
        "src/ui/widgets/ui_hybrid_list.cpp"
        "src/ui/widgets/ui_image.cpp"
        "src/ui/widgets/ui_label.cpp"
        "src/ui/widgets/ui_list.cpp"
        "src/ui/widgets/ui_menu_button.cpp"
        "src/ui/widgets/ui_paged_pane.cpp"
        "src/ui/widgets/ui_scroll_pane.cpp"
        "src/ui/widgets/ui_scrollbar.cpp"
        "src/ui/widgets/ui_scrollbar_pane.cpp"
        "src/ui/widgets/ui_slider.cpp"
        "src/ui/widgets/ui_textinput.cpp"
        )

set(HEADERS
        "include/halley/ui/halley_ui.h"
        
        "include/halley/ui/ui_anchor.h"
        "include/halley/ui/ui_data_bind.h"
        "include/halley/ui/ui_element.h"
        "include/halley/ui/ui_event.h"
        "include/halley/ui/ui_factory.h"
        "include/halley/ui/ui_factory_tester.h"
        "include/halley/ui/ui_input.h"
        "include/halley/ui/ui_painter.h"
        "include/halley/ui/ui_parent.h"
        "include/halley/ui/ui_root.h"
        "include/halley/ui/ui_sizer.h"
        "include/halley/ui/ui_style.h"
        "include/halley/ui/ui_stylesheet.h"
        "include/halley/ui/ui_validator.h"
        "include/halley/ui/ui_widget.h"

        "include/halley/ui/widgets/ui_animation.h"
        "include/halley/ui/widgets/ui_button.h"
        "include/halley/ui/widgets/ui_checkbox.h"
        "include/halley/ui/widgets/ui_clickable.h"
        "include/halley/ui/widgets/ui_dropdown.h"
        "include/halley/ui/widgets/ui_gamepad_image.h"
        "include/halley/ui/widgets/ui_hybrid_list.h"
        "include/halley/ui/widgets/ui_image.h"
        "include/halley/ui/widgets/ui_label.h"
        "include/halley/ui/widgets/ui_list.h"
        "include/halley/ui/widgets/ui_menu_button.h"
        "include/halley/ui/widgets/ui_paged_pane.h"
        "include/halley/ui/widgets/ui_scroll_pane.h"
        "include/halley/ui/widgets/ui_scrollbar.h"
        "include/halley/ui/widgets/ui_scrollbar_pane.h"
        "include/halley/ui/widgets/ui_slider.h"
        "include/halley/ui/widgets/ui_textinput.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-ui ${SOURCES} ${HEADERS})
target_link_libraries(halley-ui halley-utils halley-core)
