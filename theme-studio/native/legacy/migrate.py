"""Compile the retained 64x64 designs into independent native CWP3 applications."""
import importlib.util
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
NATIVE = ROOT.parent
PROJECT = NATIVE.parent
EXISTING = {"mario-1-1", "pacman", "gravity-maze", "gravity-liquid"}
SPECIAL = {"particle-time", "zelda-sunrise", "zootopia", "super-mario",
           "clock-tower", "time-in-words", "world-map", "spider-man-2",
           "sensor-weather-station", "sensor-comfort-garden", "sensor-climate-trend",
           "gravity-hourglass", "gravity-level", "gravity-stardust"}
TOKEN = re.compile(r'\s+|--[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|0x[0-9a-fA-F]+|(?:\d+\.\d*|\.\d+|\d+)|[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*|//|==|~=|<=|>=|\.\.|.')


class DrawCompiler:
    """A build-time translator for the small arithmetic draw-only source subset.

    Stateful designs have explicit C++ ports. Unsupported syntax fails the build.
    The installed application contains only Xtensa machine code and assets.
    """
    precedence = {"or": 1, "and": 2, "==": 3, "~=": 3, "<": 3, ">": 3,
                  "<=": 3, ">=": 3, "..": 4, "+": 5, "-": 5, "*": 6, "/": 6, "//": 6, "%": 6}

    def __init__(self, source):
        self.tokens = [t for t in TOKEN.findall(source) if not t.isspace() and not t.startswith("--")]
        self.at = 0

    def peek(self):
        return self.tokens[self.at] if self.at < len(self.tokens) else ""

    def take(self, expected=None):
        token = self.peek()
        if not token or expected and token != expected:
            raise ValueError(f"Expected {expected}, got {token} at {self.at}")
        self.at += 1
        return token

    def expression(self, priority=0):
        token = self.take()
        if token == "(":
            left = self.expression()
            self.take(")")
        elif token in {"-", "not"}:
            left = ("unary", token, self.expression(7))
        elif self.peek() == "(":
            self.take("(")
            args = []
            while self.peek() != ")":
                args.append(self.expression())
                if self.peek() != ",":
                    break
                self.take(",")
            self.take(")")
            left = ("call", token, args)
        else:
            left = ("atom", token)
        while self.precedence.get(self.peek(), -1) >= priority:
            op = self.take()
            right = self.expression(self.precedence[op] + 1)
            left = ("binary", op, left, right)
        return left

    def cpp(self, node):
        kind, token, *rest = node
        if kind == "atom":
            if token[0] in "'\"":
                text = token[1:-1]
                return "0x%04x" % rgb(text) if re.fullmatch(r"#[0-9a-fA-F]{6}", text) else json.dumps(text)
            if re.fullmatch(r"(?:\d+\.\d*|\.\d+|\d+)", token):
                return token + ("f" if "." in token else ".f")
            names = {"hm": "clock.hm", "hour": "clock.hour", "minute": "clock.minute",
                     "secondText": "clock.second", "phase": "state.phase",
                     "nextFlash": "state.nextFlash", "flashEnd": "state.flashEnd", "math.pi": "3.14159265f"}
            return names.get(token, token)
        if kind == "unary":
            return "(" + ("!" if token == "not" else token) + self.cpp(rest[0]) + ")"
        if kind == "binary":
            a, b = rest
            if token == "..":
                return "clock.hms"
            if token == "or" and a[0:2] == ("binary", "and"):
                return f"({self.cpp(a[2])}?{self.cpp(a[3])}:{self.cpp(b)})"
            left, right = self.cpp(a), self.cpp(b)
            if token == "%":
                return f"mod({left},{right})"
            if token == "//":
                return f"floorf({left}/{right})"
            return f"({left}{ {'and':'&&','or':'||','~=':'!='}.get(token,token) }{right})"
        args = [self.cpp(a) for a in rest[0]]
        if token == "clock.millis":
            return "float(state.elapsed)"
        if token == "clock.format":
            return {"%H": "clock.hour", "%M": "clock.minute", "%S": "clock.second", "%H:%M": "clock.hm", "%m/%d": "clock.date", "%m-%d": "clock.monthDash"}[rest[0][0][1][1:-1]]
        if token.startswith("math."):
            name = {"floor": "floorf", "sin": "sinf", "cos": "cosf", "abs": "fabsf", "min": "minimum", "max": "maximum", "sqrt": "sqrtf"}[token[5:]]
            return name + "(" + ",".join(args) + ")"
        if token == "rng.int":
            return "randomInt(state," + ",".join(args) + ")"
        if token in {"gfx.line", "gfx.rect"}:
            return f"api->{token[4:]}(api->context," + ",".join(args) + ")"
        if token == "gfx.text":
            return "api->text(api->context," + ",".join(args + (["1"] if len(args) == 4 else [])) + ")"
        if token == "font.draw":
            return "api->font(api->context," + ",".join(args + (["1"] if len(args) == 5 else [])) + ")"
        if token == "gfx.pixel":
            return "pixel(state,api," + ",".join(args) + ")"
        if token == "gfx.wave":
            return "wave(api," + ",".join(args) + ")"
        if token == "fx.draw":
            return "drawParticles(state,api)"
        if token == "media.draw":
            return f"state.media.draw(api,uint32_t({args[3]}))"
        raise ValueError("Unsupported draw call " + token)

    def block(self, stops=()):
        lines = []
        while self.peek() and self.peek() not in stops:
            token = self.peek()
            if token == ";":
                self.take()
            elif token == "local":
                self.take()
                name = self.take()
                self.take("=")
                lines.append("float " + name + "=" + self.cpp(self.expression()) + ";")
            elif token == "for":
                self.take()
                name = self.take()
                self.take("=")
                low = self.cpp(self.expression())
                self.take(",")
                high = self.cpp(self.expression())
                step = "1"
                if self.peek() == ",":
                    self.take()
                    step = self.cpp(self.expression())
                self.take("do")
                body = self.block(("end",))
                self.take("end")
                lines.append(f"for(float {name}={low};{name}<={high};{name}+={step}){{\n{body}\n}}")
            elif token == "if":
                self.take()
                condition = self.cpp(self.expression())
                self.take("then")
                body = self.block(("else", "end"))
                code = f"if({condition}){{\n{body}\n}}"
                if self.peek() == "else":
                    self.take()
                    code += "else{\n" + self.block(("end",)) + "\n}"
                self.take("end")
                lines.append(code)
            else:
                node = self.expression()
                if self.peek() == "=":
                    self.take()
                    lines.append(self.cpp(node) + "=" + self.cpp(self.expression()) + ";")
                elif node[0:2] == ("call", "media.draw"):
                    lines.append("if(!" + self.cpp(node) + ")return false;")
                else:
                    lines.append(self.cpp(node) + ";")
        return "\n".join(lines)


def rgb(text):
    n = int(text.lstrip("#"), 16)
    return ((n >> 19) << 11) | (((n >> 10) & 63) << 5) | ((n >> 3) & 31)


def asset_source(assets):
    lines = []
    for asset in assets:
        name = asset["id"].replace("-", "_")
        digits = 2 if asset.get("encoding", 4) == 8 else 1
        pixels = []
        for frame in asset["frames"]:
            assert len(frame) == asset["width"] * asset["height"] * digits
            pixels.extend(255 if "." in frame[i:i+digits] else int(frame[i:i+digits], 16)
                          for i in range(0, len(frame), digits))
        lines.append(f"static const uint16_t {name}_palette[]={{" + ",".join(str(rgb(c)) for c in asset["palette"]) + "};")
        lines.append(f"static const uint8_t {name}_pixels[]={{" + ",".join(map(str, pixels)) + "};")
        lines.append(f'static const Asset {name}={{"{asset["id"]}",{asset["width"]},{asset["height"]},{len(asset["frames"])},{name}_palette,{name}_pixels}};')
    return "\n".join(lines)


def regular(project):
    source = project["source"]
    body = source.split("function draw()", 1)[1].rsplit("end", 1)[0]
    draw = DrawCompiler(body).block()
    effect = re.search(r'fx.create\("atmosphere",([^\n]+)\)', source)
    fx = ""
    if effect:
        parser = DrawCompiler(effect[1])
        args = []
        while parser.peek():
            args.append(parser.cpp(parser.expression()))
            if parser.peek():
                parser.take(",")
        rain = "true" if args[-1] == '"rain"' else "false"
        fx = "particles(state,api," + ",".join(args[:-1]) + "," + rain + ");"
    setup = "state.phase=randomInt(state,0,20);" if project["id"] in {"girl", "pikachu"} else "state.phase=randomInt(state,0,2000);"
    if effect:
        setup = "state.phase=randomInt(state,0,63);state.nextFlash=randomInt(state,3,7);"
    return f"""
static void drawParticles(State& state,const CwApi* api){{{fx}}}
static bool start(void* memory,const CwApi* api){{
  auto& state=*(State*)memory;state.seed=api->random(api->context)|1;{setup}
  particlesStart(state);clip(state,api);
  return state.media.load(api,BACKGROUND_BYTES);
}}
static bool update(void* memory,const CwApi*,uint32_t dt){{((State*)memory)->elapsed+=dt;return true;}}
static bool draw(void* memory,const CwApi* api){{
  auto& state=*(State*)memory;ClockText clock(api);
  {draw}
  return true;
}}
static void button(void* memory,const CwApi*,uint32_t){{auto& state=*(State*)memory;state.phase=randomInt(state,0,2000);}}
CW_EXPORT const CwTheme* cw_theme(){{
  static const CwTheme theme={{1,sizeof(CwTheme),sizeof(State),60,start,update,draw,button,nullptr}};
  return &theme;
}}
"""


def generate():
    themes = json.loads((PROJECT / "themes/catalog.json").read_text(encoding="utf-8"))["themes"]
    folders = []
    for item in themes:
        if item["id"] in EXISTING:
            continue
        project = json.loads((PROJECT / item["path"].lstrip("/")).read_text(encoding="utf-8"))
        folder = ROOT / "themes" / project["id"]
        folder.mkdir(parents=True, exist_ok=True)
        background = project.get("media", [])
        resources = {}
        size = 0
        if background:
            data = (PROJECT / "themes/resources" / (background[0] + ".cwa")).read_bytes()
            (folder / "background.cwa").write_bytes(data)
            resources["background"] = "background.cwa"
            size = len(data)
        special = project["id"] in SPECIAL
        if special:
            body = (ROOT / "ports" / (project["id"] + ".cpp")).read_text(encoding="utf-8")
        else:
            body = regular(project)
        code = ('#define LEGACY_HAS_MEDIA\n' if background else '')
        code += '#include "../../LegacyScene.h"\nusing namespace legacy;\n'
        code += f"constexpr unsigned BACKGROUND_BYTES={size};\n"
        code += asset_source(project.get("assets", [])) + "\n" + body
        (folder / "theme.cpp").write_text(code, encoding="utf-8")
        sensors = project.get("requirements", {}).get("sensors", [])
        fps = 30 if project["id"].startswith("sensor-") else 60
        if project["id"] in {"world-map", "time-in-words"}:
            fps = 1
        elif project["id"] == "clock-tower":
            fps = 10
        manifest = {
            "id": project["id"], "packageName": "org.clockwise." + project["id"],
            "title": project["title"], "appVersion": "1.0.0",
            "description": project.get("description", item.get("description", "")).replace("Lua", "C++"),
            "author": project.get("author", "Clockwise Theme Studio"),
            "category": project.get("category", item.get("category", "")),
            "width": 64, "height": 64, "resources": resources,
            "requirements": {"chips": ["esp32s3"], "psramBytes": 0,
                             "stateBytes": 49152, "graphicsBytes": 8192,
                             "targetFps": fps, "sensors": sensors},
            "verification": {"status": "unverified"},
        }
        if project["id"] == "world-map":
            manifest["parameters"] = {
                "utcOffset": {"label": "时区 UTC 偏移 / UTC offset", "min": -12,
                              "max": 14, "step": .25, "default": 8}
            }
        (folder / "theme.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        folders.append(folder)
    return folders


def main():
    spec = importlib.util.spec_from_file_location("native_build", NATIVE / "build.py")
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    output = NATIVE / "dist"
    entries = [builder.build(folder, "esp32s3", output) for folder in generate()]
    catalog = output / "catalog.json"
    original = json.loads(catalog.read_text(encoding="utf-8"))["apps"]
    keys = {(a["id"], a["requirements"]["chips"][0]) for a in entries}
    apps = [a for a in original if (a["id"], a["requirements"]["chips"][0]) not in keys] + entries
    catalog.write_text(json.dumps({"format": 3, "apps": apps}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("MIGRATED", len(entries), "PACKAGES", sum(a["bytes"] for a in entries))


if __name__ == "__main__":
    main()
