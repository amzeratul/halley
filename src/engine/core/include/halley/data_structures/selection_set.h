#pragma once

#include <gsl/gsl_assert>
#include "vector.h"
#include "halley/utils/algorithm.h"

namespace Halley {
	enum class SelectionSetModifier {
        None,
        Add,
        Remove,
        Toggle
    };

    template <typename T>
    class SelectionSet {
    public:
        void startDrag(SelectionSetModifier modifier)
        {
            Expects(!curDrag);
            if (modifier == SelectionSetModifier::None) {
                selected.clear();
            } else {
                startSelected = selected;
            }
            curDrag = modifier;
        }

        void updateDrag(const Vector<T>& elements)
        {
            Expects(curDrag);
            if (*curDrag == SelectionSetModifier::None) {
                selected = elements;
            } else if (*curDrag == SelectionSetModifier::Add) {
                selected = startSelected;
                for (const auto& e: elements) {
	                if (!isSelected(e)) {
                        selected.push_back(e);
	                }
                }
            } else if (*curDrag == SelectionSetModifier::Remove) {
	            selected.clear();
                for (const auto& e: startSelected) {
	                if (!std_ex::contains(elements, e)) {
                        selected.push_back(e);
	                }
                }
            } else if (*curDrag == SelectionSetModifier::Toggle) {
                selected = startSelected;
                for (const auto& e: elements) {
                    const auto iter = std_ex::find(selected, e);
                    if (iter != selected.end()) {
                        selected.erase(iter);
                    } else {
                        selected.push_back(e);
                    }
                }
            }
        }

        void updateOrStartDrag(const Vector<T>& elements, SelectionSetModifier modifier)
        {
	        if (!curDrag) {
                startDrag(modifier);
	        }
            updateDrag(elements);
        }

        void endDrag()
        {
            if (curDrag) {
                startSelected.clear();
                curDrag = {};
            }
        }

        void directSelect(std::optional<T> element, SelectionSetModifier modifier)
        {
            if (modifier == SelectionSetModifier::None) {
                selected.clear();
                if (element) {
                    selected.push_back(*element);
                }
                return;
            }

            if (element) {
	            if (isSelected(*element)) {
	                if (modifier == SelectionSetModifier::Remove || modifier == SelectionSetModifier::Toggle) {
	                    std_ex::erase(selected, *element);
	                }
	            } else {
	                if (modifier == SelectionSetModifier::Add || modifier == SelectionSetModifier::Toggle) {
	                    selected.push_back(*element);
	                }
	            }
            }
        }

        void mouseButtonPressed(std::optional<T> element, SelectionSetModifier modifier, Vector2f mousePos)
        {
            if (modifier == SelectionSetModifier::None && (!element || isSelected(*element))) {
                pendingPress = PendingPress{ element, modifier, mousePos };
            } else {
                directSelect(element, modifier);
            }
        }

        void mouseButtonReleased(Vector2f mousePos)
        {
	        if (pendingPress) {
                if ((pendingPress->mousePos - mousePos).manhattanLength() < 4) {
                    directSelect(pendingPress->element, pendingPress->modifier);
                }
                pendingPress.reset();
	        }
        }

        void setSelection(gsl::span<const T> ids)
        {
            clear();
            selected.insert(selected.begin(), ids.begin(), ids.end());
        }

        void clear()
        {
            selected.clear();
        }

        bool isSelected(const T& e) const
        {
            return std_ex::contains(selected, e);
        }

        const Vector<T>& getSelected() const
        {
            return selected;
        }

    private:
        struct PendingPress {
            std::optional<T> element;
            SelectionSetModifier modifier;
            Vector2f mousePos;
        };
        Vector<T> selected;
        Vector<T> startSelected;
        std::optional<SelectionSetModifier> curDrag;
        std::optional<PendingPress> pendingPress;
    };
}
