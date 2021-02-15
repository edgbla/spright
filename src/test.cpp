
#if !defined(NDEBUG)

#include "InputParser.h"
#include "packing.h"
#include <csignal>

namespace {
  template<typename A, typename B>
  void eq(const A& a, const B& b) {
    using T = std::common_type_t<A, B>;
    if (static_cast<T>(a) != static_cast<T>(b))
      std::raise(SIGINT);
  }

  template<typename F, typename... Args>
  bool throws(F&& func, Args&&... args) try {
    func(std::forward<Args>(args)...);
    return false;
  }
  catch (...) {
    return true;
  }

  void test_tag_scopes() {
    auto input = std::stringstream(R"(
      sheet "test/Items.png"
        grid 16 16
        trim none
        tag "A"
          tag "B"
          sprite has_A_B
          trim crop
            tag "C"
            sprite has_A_B_C
              trim trim
          sprite has_A_B
        sprite has_A_D_E
          tag "D"
            trim trim
              tag "F"
            tag "E"
        tag "G"
          sprite has_A_G
    )");
    auto parser = InputParser(Settings{ });
    parser.parse(input);
    const auto& sprites = parser.sprites();
    eq(sprites.size(), 5u);

    eq(sprites[0].id, "has_A_B");
    eq(sprites[0].tags.size(), 2u);
    eq(sprites[0].trim, Trim::none);

    eq(sprites[1].id, "has_A_B_C");
    eq(sprites[1].tags.size(), 3u);
    eq(sprites[1].trim, Trim::trim);

    eq(sprites[2].id, "has_A_B");
    eq(sprites[2].tags.size(), 2u);
    eq(sprites[2].tags.count("B"), 1u);
    eq(sprites[2].trim, Trim::crop);

    eq(sprites[3].id, "has_A_D_E");
    eq(sprites[3].tags.size(), 3u);
    eq(sprites[3].tags.count("B"), 0u);
    eq(sprites[3].tags.count("E"), 1u);
    eq(sprites[3].trim, Trim::trim);

    eq(sprites[4].id, "has_A_G");
    eq(sprites[4].tags.size(), 2u);
    eq(sprites[4].tags.count("B"), 0u);
    eq(sprites[4].tags.count("G"), 1u);
    eq(sprites[4].trim, Trim::none);
  }

  void test_texture_scopes() {
    auto input = std::stringstream(R"(
      width 256
      texture "tex1"
        padding 1
      texture "tex2"
        padding 2
      width 128
      texture "tex3"
        padding 3
      width 64
      sheet "test/Items.png"
        grid 16 16
        sprite
        sprite
          texture "tex1"
        sprite
          texture "tex2"
        sprite
    )");
    auto parser = InputParser(Settings{ .autocomplete = true });
    parser.parse(input);
    const auto& sprites = parser.sprites();
    eq(sprites.size(), 4);
    eq(sprites[0].texture->border_padding, 3);
    eq(sprites[1].texture->border_padding, 1);
    eq(sprites[2].texture->border_padding, 2);
    eq(sprites[0].texture, sprites[3].texture);
    eq(sprites[0].texture->width, 128);
    eq(sprites[1].texture->width, 256);
    eq(sprites[2].texture->width, 256);
  }

  void test_grid_autocompletion() {
    auto input = std::stringstream(R"(
      sheet "test/Items.png"
        grid 16 16
    )");
    auto parser = InputParser(Settings{ .autocomplete = true });
    parser.parse(input);
    const auto& sprites = parser.sprites();
    eq(sprites.size(), 18);
  }

  void test_unaligned_autocompletion() {
    auto input = std::stringstream(R"(
      sheet "test/Items.png"
    )");
    auto parser = InputParser(Settings{ .autocomplete = true });
    parser.parse(input);
    const auto& sprites = parser.sprites();
    eq(sprites.size(), 31);
  }

  void test_packing() {
    const auto pack = [](const char* definition) {
      auto input = std::stringstream(definition);
      auto parser = InputParser(Settings{ .autocomplete = true });
      parser.parse(input);
      auto sprites = std::move(parser).sprites();
      return pack_sprites(sprites);
    };
    auto textures = pack(R"(
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 64);
    eq(textures[0].height, 61);

    textures = pack(R"(
      allow-rotate true
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 64);
    eq(textures[0].height, 59);

    textures = pack(R"(
      deduplicate true
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 63);
    eq(textures[0].height, 54);

    textures = pack(R"(
      allow-rotate true
      deduplicate true
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 55);
    eq(textures[0].height, 64);

    textures = pack(R"(
      max-width 128
      max-height 128
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 64);
    eq(textures[0].height, 61);

    textures = pack(R"(
      width 128
      max-height 128
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 128);
    eq(textures[0].height, 37);

    textures = pack(R"(
      max-width 128
      height 128
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 64);
    eq(textures[0].height, 128);

    textures = pack(R"(
      max-width 40
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 40);
    eq(textures[0].height, 86);

    textures = pack(R"(
      max-height 40
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 88);
    eq(textures[0].height, 40);

    textures = pack(R"(
      power-of-two true
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 64);
    eq(textures[0].height, 64);

    textures = pack(R"(
      padding 1
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 72);
    eq(textures[0].height, 60);

    textures = pack(R"(
      padding 1
      power-of-two true
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 128);
    eq(textures[0].height, 64);

    textures = pack(R"(
      max-width 40
      max-height 40
      sheet "test/Items.png"
    )");
    eq(textures.size(), 3);
    eq(textures[0].width, 40);
    eq(textures[0].height, 40);
    eq(textures[1].width, 32);
    eq(textures[1].height, 40);
    eq(textures[2].width, 20);
    eq(textures[2].height, 30);

    textures = pack(R"(
      max-width 40
      max-height 40
      power-of-two true
      sheet "test/Items.png"
    )");
    eq(textures.size(), 4);
    eq(textures[0].width, 32);
    eq(textures[0].height, 32);
    eq(textures[1].width, 32);
    eq(textures[1].height, 32);
    eq(textures[2].width, 32);
    eq(textures[2].height, 32);
    eq(textures[3].width, 32);
    eq(textures[3].height, 16);

    textures = pack("");
    eq(textures.size(), 0);

    textures = pack("padding 1");
    eq(textures.size(), 0);

    textures = pack(R"(
      max-width 16
      max-height 16
      sheet "test/Items.png"
    )");
    eq(textures.size(), 14);

    eq(throws(pack, R"(
      padding 1
      max-width 16
      max-height 16
      sheet "test/Items.png"
    )"), true);

    textures = pack(R"(
      max-height 16
      common-divisor 16
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 496);
    eq(textures[0].height, 16);

    textures = pack(R"(
      max-height 30
      common-divisor 24
      extrude 1
      sheet "test/Items.png"
    )");
    eq(textures.size(), 1);
    eq(textures[0].width, 806);
    eq(textures[0].height, 26);
  }
} // namespace

void test() {
  auto error = std::error_code();
  if (!std::filesystem::exists("test/Items.png", error))
    return;

  test_tag_scopes();
  test_texture_scopes();
  test_grid_autocompletion();
  test_unaligned_autocompletion();
  test_packing();
}

#endif // !defined(NDEBUG)
