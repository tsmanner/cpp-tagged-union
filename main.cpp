#include <stdio.h>
#include "include/TaggedUnion.h"


struct RelativeMove {
    int32_t delta_x;
    int32_t delta_y;
};

struct Button {
    uint16_t code;
    bool down;
};

struct Scroll {
    int32_t delta_x;
    int32_t delta_y;
};


DEFINE_TAGGED_UNION(MouseEvent, RelativeMove, Button, Scroll);


void examplePrintEventIndexed(MouseEvent const &event) {
    switch (event.activeIndex()) {
    case MouseEvent::indexOf<RelativeMove>(): {
        auto relative_move = event.as<RelativeMove>();
        printf("move %d x %d\n", relative_move.delta_x, relative_move.delta_y);
        break;
    }
    case MouseEvent::indexOf<Button>(): {
        auto button = event.as<Button>();
        printf("button code=%d down=%d\n", button.code, button.down);
        break;
    }
    case MouseEvent::indexOf<Scroll>(): {
        auto scroll = event.as<Scroll>();
        printf("scroll %d x %d\n", scroll.delta_x, scroll.delta_y);
        break;
    }
    }
}

void examplePrintEventKind(MouseEvent const &event) {
    switch (event.kind()) {
    case MouseEvent::Kind::RelativeMove: {
        auto relative_move = event.as<RelativeMove>();
        printf("move %d x %d\n", relative_move.delta_x, relative_move.delta_y);
        break;
    }
    case MouseEvent::Kind::Button: {
        auto button = event.as<Button>();
        printf("button code=%d down=%d\n", button.code, button.down);
        break;
    }
    case MouseEvent::Kind::Scroll: {
        auto scroll = event.as<Scroll>();
        printf("scroll %d x %d\n", scroll.delta_x, scroll.delta_y);
        break;
    }
    }
}

int main() {
  examplePrintEventIndexed(MouseEvent::create<RelativeMove>(1, 0));
  examplePrintEventIndexed(MouseEvent::create<Button>(uint16_t(123), true));
  examplePrintEventIndexed(MouseEvent::create<Scroll>(-2, 3));
  examplePrintEventKind(MouseEvent::create<RelativeMove>(1, 0));
  examplePrintEventKind(MouseEvent::create<Button>(uint16_t(123), true));
  examplePrintEventKind(MouseEvent::create<Scroll>(-2, 3));
  try {
    MouseEvent::create<Scroll>(-2, 3).as<Button>();
  } catch (char const *e) {
    printf("%s\n", e);
  }
  return 0;
}
