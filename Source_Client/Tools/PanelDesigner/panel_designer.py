"""
Panel9 Layout Designer - standalone desktop version.

Loads the REAL deployed game assets (Data\\Custom\\Panel9\\stone.OZJ,
Data\\Custom\\Common\\close_btn_x.OZT) directly via mu_decode.py - no PNG
export step, no browser, no dependency on anything but this folder's own
Python files and a normal Python install (Pillow + numpy: `pip install
pillow numpy`).

Run it:
    python panel_designer.py

Drag the gold handle to resize the panel, drag the X to reposition it, or
drag any thumbnail from the "Biblioteca" panel on the right (every .OZT/
.OZJ in Data\\Interface - the game's own native UI art, 600+ files) onto:
  - the small "Fundo" swatch in the sidebar - swaps the panel's own
    background texture (auto-fits the panel to the dropped image's size)
  - the close button on the canvas - swaps that texture
  - anywhere else on the panel body - adds a new CLICKABLE ELEMENT (a
    button) there, draggable/resizable/deletable independently, listed in
    the "Elementos" panel in the sidebar
Then either:
  - "Copiar codigo" - puts the Lua snippet on the clipboard, paste it into
    Panel9Demo.lua by hand, or
  - "Salvar e implantar" - patches Panel9Demo.lua directly (regex, only the
    exact lines this tool owns - see patch_lua_file() below) and re-encrypts
    it straight to Global Debug\\Lua\\Panel9Demo.enc (same XOR format
    encrypt_lua.py uses, reimplemented inline here rather than shelling out
    to it - see deploy() - so this step has no dependency on a system
    Python even once packaged as an .exe), so the only remaining step is
    relaunching Main.exe to see it in-game.

    Note: dropping a library asset only changes what's previewed/exported
    as `Panel9Demo_StoneTex`/`Panel9Demo_CloseTex` conceptually - "Salvar e
    implantar" only ever patches the position/size lines this tool has
    always owned, not the LoadImage() paths themselves (those point at
    Data\\Custom\\..., not Data\\Interface\\...). If a dropped native asset
    is worth keeping, copy the matching .OZT/.OZJ into Data\\Custom\\ and
    update the two LoadImage() calls in Panel9Demo.lua by hand - not
    automated, deliberately, since it'd mean guessing which of the two slots
    (stone vs close) and what filename to give it.

No build step needed to just RUN this (it's Python, not C++) - Visual
Studio can still open and run it directly if you'd rather work there:
File > Open > Folder on Tools\\PanelDesigner, install the "Python
Development" workload if it isn't already there, then just Start/F5 like
any other project.

To get an actual double-click .exe instead (no Python install needed to
run it, though building it still needs one): from this folder,
    pip install pyinstaller
    pyinstaller --onefile --windowed --paths "../ImageConverter" --name Panel9LayoutDesigner panel_designer.py
The .exe lands in dist\\Panel9LayoutDesigner.exe - copy it back into this
same Tools\\PanelDesigner\\ folder (it locates every other file relative to
its own location, `sys.executable` when frozen - see HERE below - so it
has to stay inside the project tree, just not necessarily this exact
subfolder as long as the relative layout to Data\\/Lua-Source\\ is intact).
"""
import os
import re
import sys
import threading
import tkinter as tk
from tkinter import messagebox

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ImageConverter"))
from mu_decode import load_mu_image  # noqa: E402

from PIL import Image, ImageTk  # noqa: E402

# ---------------------------------------------------------------------------
# Paths - all relative to this file/exe, so the tool works regardless of
# where the project folder is cloned/renamed to. PyInstaller's --onefile
# mode unpacks to a temp dir at runtime, so __file__ inside a frozen build
# points there, not at the real .exe on disk - sys.executable is the real
# path in that case instead (see PyInstaller's own docs on sys.frozen).
# ---------------------------------------------------------------------------
if getattr(sys, "frozen", False):
    HERE = os.path.dirname(os.path.abspath(sys.executable))
else:
    HERE = os.path.dirname(os.path.abspath(__file__))

SOURCE_CLIENT = os.path.join(HERE, "..", "..")
DATA_CUSTOM = os.path.join(SOURCE_CLIENT, "Global Debug", "Data", "Custom")
DATA_INTERFACE = os.path.join(SOURCE_CLIENT, "Global Debug", "Data", "Interface")
SCREENSHOTS_DIR = os.path.join(SOURCE_CLIENT, "Global Debug", "Screenshots")
LUA_SOURCE = os.path.join(SOURCE_CLIENT, "Lua-Source")
PANEL9DEMO_LUA = os.path.join(LUA_SOURCE, "Panel9Demo.lua")
DEPLOY_DIR = os.path.join(SOURCE_CLIENT, "Global Debug", "Lua")

STONE_PATH = os.path.join(DATA_CUSTOM, "Panel9", "stone.OZJ")
CLOSE_PATH = os.path.join(DATA_CUSTOM, "Common", "close_btn_x.OZT")

# Same key/magic as Lua-Source\encrypt_lua.py - duplicated here (not
# imported) so the deploy step works standalone even in a frozen .exe with
# no system Python available to shell out to.
_ENCRYPT_KEY = bytes([
    0x52, 0x65, 0x62, 0x69, 0x72, 0x74, 0x68, 0x4D,
    0x55, 0x32, 0x30, 0x32, 0x36, 0x21, 0x21, 0x21
])
_ENCRYPT_MAGIC = bytes([0x52, 0x50, 0x4D, 0x55, 0x01])  # "RPMU" + 0x01

LOGICAL_W, LOGICAL_H = 640, 480  # same virtual canvas ScreenWidth()/Height() use
SCALE = 1.3

# Interactive resizing/dragging re-runs PIL resize() + a fresh PhotoImage
# every mouse-motion event - LANCZOS (the default elsewhere in this
# project's tools, chosen there for one-off export quality) is too slow
# for that many times a second, and resizing straight from a 900-1600px
# source made it worse. Two fixes: cap the *source* image once at load
# time (below), and use BILINEAR (much cheaper, still smooth enough at
# this preview's on-screen size) for the live per-frame resize.
LIVE_RESAMPLE = Image.BILINEAR
_SOURCE_CAP = 480  # longest side, after loading, before any interactive resizing
THUMB_SIZE = 48


def _cap_size(img, cap=_SOURCE_CAP):
    if max(img.size) <= cap:
        return img
    img = img.copy()
    img.thumbnail((cap, cap), Image.LANCZOS)  # one-time cost, not per-frame
    return img


def latest_screenshot():
    if not os.path.isdir(SCREENSHOTS_DIR):
        return None
    shots = [f for f in os.listdir(SCREENSHOTS_DIR) if f.lower().endswith((".jpg", ".jpeg", ".png"))]
    if not shots:
        return None
    shots.sort(key=lambda f: os.path.getmtime(os.path.join(SCREENSHOTS_DIR, f)), reverse=True)
    return os.path.join(SCREENSHOTS_DIR, shots[0])


def list_interface_assets():
    if not os.path.isdir(DATA_INTERFACE):
        return []
    names = [f for f in os.listdir(DATA_INTERFACE) if f.lower().endswith((".ozt", ".ozj"))]
    names.sort(key=str.lower)
    return names


def patch_lua_file(panel_w, panel_h, close_size, off_x, off_y):
    """
    Rewrites just the lines this tool owns in Panel9Demo.lua - the panel
    size line, the close-size line, and BOTH copies of the cx/cy offset
    math (OnMainProc and OnClickEvent each compute it inline - there's no
    shared function to patch once). Counts every substitution and refuses
    to write anything if a count doesn't match what's expected, so a
    structural change to the file (a rewrite that changes this shape)
    fails loudly instead of silently mangling the file.
    """
    with open(PANEL9DEMO_LUA, "r", encoding="utf-8") as f:
        text = f.read()

    text, n1 = re.subn(
        r"Panel9Demo_PanelW, Panel9Demo_PanelH, Panel9Demo_CornerSize = \d+, \d+, (\d+)",
        lambda m: f"Panel9Demo_PanelW, Panel9Demo_PanelH, Panel9Demo_CornerSize = {panel_w}, {panel_h}, {m.group(1)}",
        text,
    )
    text, n2 = re.subn(
        r"Panel9Demo_CloseSize = \d+",
        f"Panel9Demo_CloseSize = {close_size}",
        text,
    )
    text, n3 = re.subn(
        r"(local cx = x \+ Panel9Demo_PanelW - Panel9Demo_CloseSize \* )[\d.]+",
        lambda m: f"{m.group(1)}{off_x:.2f}",
        text,
    )
    text, n4 = re.subn(
        r"(local cy = y - Panel9Demo_CloseSize \* )[\d.]+",
        lambda m: f"{m.group(1)}{off_y:.2f}",
        text,
    )

    expected = {"panel size line": (n1, 1), "close size line": (n2, 1),
                "cx offset (both copies)": (n3, 2), "cy offset (both copies)": (n4, 2)}
    bad = [name for name, (got, want) in expected.items() if got != want]
    if bad:
        raise RuntimeError(
            "Panel9Demo.lua doesn't match the shape this tool expects (" + ", ".join(bad) +
            ") - nothing was written. The file structure probably changed; patch it by hand this time."
        )

    with open(PANEL9DEMO_LUA, "w", encoding="utf-8") as f:
        f.write(text)


def deploy():
    """
    Same format encrypt_lua.py writes (XOR against _ENCRYPT_KEY, repeating,
    prefixed with _ENCRYPT_MAGIC) - reimplemented here instead of shelling
    out to that script so this works with no system Python present, which
    matters once this tool is a frozen .exe (see the module docstring).
    """
    with open(PANEL9DEMO_LUA, "rb") as f:
        data = f.read()

    encrypted = bytearray(data)
    for i in range(len(encrypted)):
        encrypted[i] ^= _ENCRYPT_KEY[i % len(_ENCRYPT_KEY)]

    os.makedirs(DEPLOY_DIR, exist_ok=True)
    out_path = os.path.join(DEPLOY_DIR, "Panel9Demo.enc")
    with open(out_path, "wb") as f:
        f.write(_ENCRYPT_MAGIC + bytes(encrypted))

    return f"{len(data)} bytes -> {out_path}"


class App:
    FG = "#ece4d3"
    DIM = "#a89a7f"
    SURFACE = "#1d1911"
    SURFACE2 = "#272016"
    ACCENT = "#c9a44c"
    BORDER = "#3c3222"

    def __init__(self, root):
        self.root = root
        root.title("Panel9 Layout Designer")
        root.configure(bg="#14120e")

        self.panel_w = 260
        self.panel_h = 320
        self.close_size = 26
        self.off_x = 0.7
        self.off_y = 0.3
        self.drag = None

        self.lib_thumbs = {}       # filename -> ImageTk.PhotoImage (48x48)
        self.lib_drag_name = None  # filename currently being dragged out of the library
        self.lib_drag_float = None  # the little Toplevel following the cursor

        # Clickable elements placed on top of the panel (e.g. buttons) -
        # each is {"id", "name", "pil", "x", "y", "w", "h"}, with x/y/w/h
        # all stored RELATIVE to the panel's own top-left corner (not
        # absolute canvas coordinates), so an element stays put on the
        # panel if the panel itself gets resized/recentered later.
        self.elements = []
        self.next_element_id = 1
        self.selected_element_id = None
        self.element_photos = {}      # element id -> ImageTk.PhotoImage (current frame)
        self._element_drag_offset = (0, 0)  # mouse-to-element-corner offset, captured at press time

        self.history = []  # list of {"label": str, "state": {...}} - see push_history()/restore_history()

        self._load_images()
        self._build_ui()
        self.push_history("Padrao inicial")
        self.redraw()
        self._start_library_scan()

    # ---- assets -----------------------------------------------------

    def _load_images(self):
        self.stone_pil = _cap_size(load_mu_image(STONE_PATH))
        self.close_pil = _cap_size(load_mu_image(CLOSE_PATH))

        shot_path = latest_screenshot()
        if shot_path:
            bg = Image.open(shot_path).convert("RGB")
        else:
            bg = Image.new("RGB", (LOGICAL_W, LOGICAL_H), (40, 44, 52))
        stage_w, stage_h = int(LOGICAL_W * SCALE), int(LOGICAL_H * SCALE)
        self.bg_photo = ImageTk.PhotoImage(bg.resize((stage_w, stage_h), Image.LANCZOS))
        self.shot_name = os.path.basename(shot_path) if shot_path else "(nenhum print encontrado, fundo liso)"

    # ---- UI -----------------------------------------------------------

    def _build_ui(self):
        MONO = ("Consolas", 10)

        outer = tk.Frame(self.root, bg="#14120e")
        outer.pack(padx=16, pady=16)

        title = tk.Label(outer, text="Panel9 Layout Designer", fg=self.FG, bg="#14120e",
                          font=("Segoe UI", 14, "bold"))
        title.grid(row=0, column=0, columnspan=3, sticky="w")

        sub = tk.Label(outer, text=f"fundo: {self.shot_name}", fg=self.DIM, bg="#14120e",
                        font=("Segoe UI", 9))
        sub.grid(row=1, column=0, columnspan=3, sticky="w", pady=(0, 10))

        stage_w, stage_h = int(LOGICAL_W * SCALE), int(LOGICAL_H * SCALE)
        self.canvas = tk.Canvas(outer, width=stage_w, height=stage_h,
                                 bg="black", highlightthickness=1, highlightbackground=self.BORDER)
        self.canvas.grid(row=2, column=0, sticky="n")
        self.canvas.bind("<ButtonPress-1>", self.on_press)
        self.canvas.bind("<B1-Motion>", self.on_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)

        side = tk.Frame(outer, bg=self.SURFACE, padx=14, pady=14)
        side.grid(row=2, column=1, sticky="n", padx=(16, 0))

        def field(parent, label_text, var, row):
            tk.Label(parent, text=label_text, fg=self.DIM, bg=self.SURFACE, font=("Segoe UI", 9)).grid(
                row=row, column=0, sticky="w", pady=4)
            e = tk.Entry(parent, textvariable=var, width=8, bg=self.SURFACE2, fg=self.FG,
                          insertbackground=self.FG, relief="flat", font=MONO, justify="right")
            e.grid(row=row, column=1, sticky="e", pady=4)
            return e

        tk.Label(side, text="PAINEL", fg=self.ACCENT, bg=self.SURFACE, font=("Segoe UI", 9, "bold")).grid(
            row=0, column=0, columnspan=2, sticky="w", pady=(0, 6))

        self.var_w = tk.IntVar(value=self.panel_w)
        self.var_h = tk.IntVar(value=self.panel_h)
        field(side, "Largura (px)", self.var_w, 1)
        field(side, "Altura (px)", self.var_h, 2)
        self.var_w.trace_add("write", self.on_entry_change)
        self.var_h.trace_add("write", self.on_entry_change)

        # Dedicated drop target for swapping the panel's own background
        # texture - separate from the panel body on the main canvas, which
        # now means "drop here to add a clickable element" instead (see
        # _hit_test_screen_point()'s own comment on why these two needed
        # to split apart).
        bg_row = tk.Frame(side, bg=self.SURFACE)
        bg_row.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(6, 0))
        tk.Label(bg_row, text="Fundo (arraste aqui p/ trocar)", fg=self.DIM, bg=self.SURFACE,
                  font=("Segoe UI", 8), wraplength=170, justify="left").pack(side="left", padx=(0, 6))
        self.bg_swatch = tk.Canvas(bg_row, width=36, height=36, bg=self.SURFACE2,
                                    highlightthickness=1, highlightbackground=self.ACCENT)
        self.bg_swatch.pack(side="right")

        tk.Frame(side, bg=self.BORDER, height=1).grid(row=3, column=0, columnspan=2, sticky="ew", pady=10)

        tk.Label(side, text="BOTAO DE FECHAR", fg=self.ACCENT, bg=self.SURFACE, font=("Segoe UI", 9, "bold")).grid(
            row=4, column=0, columnspan=2, sticky="w", pady=(0, 6))
        self.var_close = tk.IntVar(value=self.close_size)
        field(side, "Tamanho (px)", self.var_close, 5)
        self.var_close.trace_add("write", self.on_entry_change)

        tk.Frame(side, bg=self.BORDER, height=1).grid(row=6, column=0, columnspan=2, sticky="ew", pady=10)

        tk.Label(side, text="CODIGO LUA", fg=self.ACCENT, bg=self.SURFACE, font=("Segoe UI", 9, "bold")).grid(
            row=7, column=0, columnspan=2, sticky="w", pady=(0, 6))
        self.code_text = tk.Text(side, width=34, height=9, bg="#0e0c08", fg="#d7cdb4",
                                  insertbackground="#d7cdb4", relief="flat", font=("Consolas", 9), wrap="none")
        self.code_text.grid(row=8, column=0, columnspan=2, sticky="ew")

        btn_row = tk.Frame(side, bg=self.SURFACE)
        btn_row.grid(row=9, column=0, columnspan=2, sticky="ew", pady=(10, 0))

        copy_btn = tk.Button(btn_row, text="Copiar codigo", command=self.copy_code,
                              bg="#7c6229", fg="#f4e9c8", relief="flat", font=("Segoe UI", 9, "bold"),
                              activebackground="#8f7130", padx=8, pady=6)
        copy_btn.pack(fill="x", pady=(0, 6))

        save_btn = tk.Button(btn_row, text="Salvar e implantar", command=self.save_and_deploy,
                              bg="#4a6b3a", fg="#e3f0da", relief="flat", font=("Segoe UI", 9, "bold"),
                              activebackground="#5a7d48", padx=8, pady=6)
        save_btn.pack(fill="x")

        self.status_label = tk.Label(side, text="", fg=self.DIM, bg=self.SURFACE, font=("Segoe UI", 8),
                                      wraplength=260, justify="left")
        self.status_label.grid(row=10, column=0, columnspan=2, sticky="w", pady=(8, 0))

        tk.Frame(side, bg=self.BORDER, height=1).grid(row=11, column=0, columnspan=2, sticky="ew", pady=10)

        tk.Label(side, text="HISTORICO", fg=self.ACCENT, bg=self.SURFACE, font=("Segoe UI", 9, "bold")).grid(
            row=12, column=0, columnspan=2, sticky="w", pady=(0, 4))
        tk.Label(side, text="Clique num item pra voltar pra ele.", fg=self.DIM, bg=self.SURFACE,
                  font=("Segoe UI", 8)).grid(row=13, column=0, columnspan=2, sticky="w", pady=(0, 6))

        hist_frame = tk.Frame(side, bg=self.SURFACE)
        hist_frame.grid(row=14, column=0, columnspan=2, sticky="ew")
        hist_scroll = tk.Scrollbar(hist_frame, orient="vertical")
        self.history_list = tk.Listbox(hist_frame, height=7, bg=self.SURFACE2, fg=self.FG,
                                        selectbackground=self.ACCENT, selectforeground="#14120e",
                                        relief="flat", font=("Segoe UI", 9), activestyle="none",
                                        yscrollcommand=hist_scroll.set, exportselection=False)
        hist_scroll.configure(command=self.history_list.yview)
        self.history_list.pack(side="left", fill="both", expand=True)
        hist_scroll.pack(side="right", fill="y")
        self.history_list.bind("<<ListboxSelect>>", self.on_history_select)

        remove_btn = tk.Button(side, text="Remover selecionado", command=self.remove_history_selected,
                                bg=self.SURFACE2, fg=self.DIM, relief="flat", font=("Segoe UI", 8),
                                activebackground=self.BORDER, padx=6, pady=4)
        remove_btn.grid(row=15, column=0, columnspan=2, sticky="ew", pady=(6, 0))

        tk.Frame(side, bg=self.BORDER, height=1).grid(row=16, column=0, columnspan=2, sticky="ew", pady=10)

        tk.Label(side, text="ELEMENTOS (BOTOES)", fg=self.ACCENT, bg=self.SURFACE, font=("Segoe UI", 9, "bold")).grid(
            row=17, column=0, columnspan=2, sticky="w", pady=(0, 4))
        tk.Label(side, text="Arraste um asset da biblioteca em\ncima do painel pra criar um botao.", fg=self.DIM,
                  bg=self.SURFACE, font=("Segoe UI", 8), justify="left").grid(
            row=18, column=0, columnspan=2, sticky="w", pady=(0, 6))

        elem_frame = tk.Frame(side, bg=self.SURFACE)
        elem_frame.grid(row=19, column=0, columnspan=2, sticky="ew")
        elem_scroll = tk.Scrollbar(elem_frame, orient="vertical")
        self.element_list = tk.Listbox(elem_frame, height=5, bg=self.SURFACE2, fg=self.FG,
                                        selectbackground=self.ACCENT, selectforeground="#14120e",
                                        relief="flat", font=("Segoe UI", 9), activestyle="none",
                                        yscrollcommand=elem_scroll.set, exportselection=False)
        elem_scroll.configure(command=self.element_list.yview)
        self.element_list.pack(side="left", fill="both", expand=True)
        elem_scroll.pack(side="right", fill="y")
        self.element_list.bind("<<ListboxSelect>>", self.on_element_list_select)

        elem_remove_btn = tk.Button(side, text="Remover elemento", command=self.remove_selected_element,
                                     bg=self.SURFACE2, fg=self.DIM, relief="flat", font=("Segoe UI", 8),
                                     activebackground=self.BORDER, padx=6, pady=4)
        elem_remove_btn.grid(row=20, column=0, columnspan=2, sticky="ew", pady=(6, 0))

        self._build_library_panel(outer)

    def _build_library_panel(self, outer):
        stage_h = int(LOGICAL_H * SCALE)
        lib = tk.Frame(outer, bg=self.SURFACE, padx=10, pady=10)
        lib.grid(row=2, column=2, sticky="n", padx=(16, 0))

        tk.Label(lib, text="BIBLIOTECA", fg=self.ACCENT, bg=self.SURFACE, font=("Segoe UI", 9, "bold")).pack(
            anchor="w")
        tk.Label(lib, text="Data\\Interface - arraste no painel\nou no botao de fechar", fg=self.DIM,
                  bg=self.SURFACE, font=("Segoe UI", 8), justify="left").pack(anchor="w", pady=(0, 8))

        self.lib_status = tk.Label(lib, text="carregando...", fg=self.DIM, bg=self.SURFACE, font=("Segoe UI", 8))
        self.lib_status.pack(anchor="w", pady=(0, 6))

        lib_canvas_frame = tk.Frame(lib, bg=self.SURFACE)
        lib_canvas_frame.pack()

        cols = 4
        lib_w = cols * (THUMB_SIZE + 10) + 10
        self.lib_canvas = tk.Canvas(lib_canvas_frame, width=lib_w, height=stage_h - 40,
                                     bg=self.SURFACE2, highlightthickness=1, highlightbackground=self.BORDER)
        scrollbar = tk.Scrollbar(lib_canvas_frame, orient="vertical", command=self.lib_canvas.yview)
        self.lib_canvas.configure(yscrollcommand=scrollbar.set)
        self.lib_canvas.pack(side="left")
        scrollbar.pack(side="right", fill="y")

        self.lib_canvas.bind("<MouseWheel>", self._on_lib_scroll)
        self.lib_canvas.bind("<Button-4>", lambda e: self.lib_canvas.yview_scroll(-2, "units"))
        self.lib_canvas.bind("<Button-5>", lambda e: self.lib_canvas.yview_scroll(2, "units"))

        self.lib_cols = cols

    def _on_lib_scroll(self, event):
        self.lib_canvas.yview_scroll(-1 if event.delta > 0 else 1, "units")

    # ---- library loading (background thread) ---------------------------

    def _start_library_scan(self):
        thread = threading.Thread(target=self._scan_library_worker, daemon=True)
        thread.start()

    def _scan_library_worker(self):
        names = list_interface_assets()
        total = len(names)
        for i, name in enumerate(names):
            try:
                path = os.path.join(DATA_INTERFACE, name)
                img = load_mu_image(path)
                thumb = img.copy()
                thumb.thumbnail((THUMB_SIZE, THUMB_SIZE), Image.LANCZOS)
                # Composite onto a fixed-size dark square so every grid cell
                # lines up regardless of the source's aspect ratio.
                cell = Image.new("RGBA", (THUMB_SIZE, THUMB_SIZE), (39, 32, 22, 255))
                off = ((THUMB_SIZE - thumb.width) // 2, (THUMB_SIZE - thumb.height) // 2)
                cell.paste(thumb, off, thumb if thumb.mode == "RGBA" else None)
                self.root.after(0, self._add_library_thumb, name, cell, i + 1, total)
            except Exception:
                continue  # a handful of native assets use formats/quirks this tool doesn't parse - skip, not fatal

    def _add_library_thumb(self, name, cell_img, done, total):
        photo = ImageTk.PhotoImage(cell_img)
        self.lib_thumbs[name] = photo

        idx = len(self.lib_thumbs) - 1
        col = idx % self.lib_cols
        row = idx // self.lib_cols
        x = 10 + col * (THUMB_SIZE + 10)
        y = 10 + row * (THUMB_SIZE + 10)

        item = self.lib_canvas.create_image(x, y, anchor="nw", image=photo, tags=("thumb",))
        self.lib_canvas.tag_bind(item, "<ButtonPress-1>", lambda e, n=name: self._lib_drag_start(n, e))

        self.lib_canvas.configure(scrollregion=(0, 0, 0, y + THUMB_SIZE + 10))
        self.lib_status.config(text=f"{done}/{total} carregados" if done < total else f"{total} assets")

    # ---- geometry -------------------------------------------------------

    def panel_rect(self):
        x = (LOGICAL_W - self.panel_w) / 2
        y = (LOGICAL_H - self.panel_h) / 2
        return x, y, self.panel_w, self.panel_h

    def close_rect(self):
        x, y, w, h = self.panel_rect()
        cx = x + w - self.close_size * self.off_x
        cy = y - self.close_size * self.off_y
        return cx, cy, self.close_size, self.close_size

    # ---- drawing --------------------------------------------------------

    def redraw(self):
        c = self.canvas
        c.delete("all")
        c.create_image(0, 0, anchor="nw", image=self.bg_photo)

        x, y, w, h = self.panel_rect()
        px, py, pw, ph = int(x * SCALE), int(y * SCALE), int(w * SCALE), int(h * SCALE)
        pw, ph = max(pw, 4), max(ph, 4)
        stone_resized = self.stone_pil.resize((pw, ph), LIVE_RESAMPLE)
        self.stone_photo = ImageTk.PhotoImage(stone_resized)  # keep a ref, tkinter drops GC'd images
        c.create_image(px, py, anchor="nw", image=self.stone_photo)
        c.create_rectangle(px, py, px + pw, py + ph, outline="#e8c778", dash=(3, 3), width=1)

        cx, cy, cw, ch = self.close_rect()
        cpx, cpy, cpw, cph = int(cx * SCALE), int(cy * SCALE), int(cw * SCALE), int(ch * SCALE)
        cpw, cph = max(cpw, 4), max(cph, 4)
        close_resized = self.close_pil.resize((cpw, cph), LIVE_RESAMPLE)
        self.close_photo = ImageTk.PhotoImage(close_resized)
        c.create_image(cpx, cpy, anchor="nw", image=self.close_photo, tags="closebtn")

        # Elements (buttons) - drawn on top of the panel+close, below the
        # resize handle. The selected one gets a dashed outline + its own
        # small resize handle, matching the visual language the panel's
        # own handle already established.
        self.element_photos = {}
        for el in self.elements:
            ex = int((x + el["x"]) * SCALE)
            ey = int((y + el["y"]) * SCALE)
            ew = max(int(el["w"] * SCALE), 4)
            eh = max(int(el["h"] * SCALE), 4)
            resized = el["pil"].resize((ew, eh), LIVE_RESAMPLE)
            photo = ImageTk.PhotoImage(resized)
            self.element_photos[el["id"]] = photo
            c.create_image(ex, ey, anchor="nw", image=photo, tags=("element", f"element{el['id']}"))

            if el["id"] == self.selected_element_id:
                c.create_rectangle(ex, ey, ex + ew, ey + eh, outline="#8ecae6", dash=(3, 3), width=1)
                eh_r = 7
                c.create_oval(ex + ew - eh_r, ey + eh - eh_r, ex + ew + eh_r, ey + eh + eh_r,
                              fill="#8ecae6", outline="#0c0a07", width=2, tags="element_handle")

        handle_r = 9
        hx, hy = px + pw, py + ph
        c.create_oval(hx - handle_r, hy - handle_r, hx + handle_r, hy + handle_r,
                      fill="#c9a44c", outline="#0c0a07", width=2, tags="handle")

        self.var_w.set(int(self.panel_w))
        self.var_h.set(int(self.panel_h))
        self.var_close.set(int(self.close_size))

        bg_thumb = self.stone_pil.copy()
        bg_thumb.thumbnail((36, 36), Image.LANCZOS)
        self.bg_swatch_photo = ImageTk.PhotoImage(bg_thumb)
        self.bg_swatch.delete("all")
        self.bg_swatch.create_image(18, 18, image=self.bg_swatch_photo)

        code = (
            f"Panel9Demo_PanelW, Panel9Demo_PanelH, Panel9Demo_CornerSize = "
            f"{int(self.panel_w)}, {int(self.panel_h)}, 48\n\n"
            f"Panel9Demo_CloseSize = {int(self.close_size)}\n\n"
            f"-- (OnMainProc e OnClickEvent, mesma formula nos dois lugares)\n"
            f"local cx = x + Panel9Demo_PanelW - Panel9Demo_CloseSize * {self.off_x:.2f}\n"
            f"local cy = y - Panel9Demo_CloseSize * {self.off_y:.2f}"
        )

        if self.elements:
            code += "\n\n-- Botoes (adicione manualmente - ver comentario no topo do arquivo)\nPanel9Demo_Elements = {\n"
            for el in self.elements:
                lua_path = os.path.splitext(el["name"])[0]
                code += (
                    f'\t{{ tex = LoadImage("Interface\\\\{lua_path}"), '
                    f'x = {el["x"]:.0f}, y = {el["y"]:.0f}, w = {el["w"]:.0f}, h = {el["h"]:.0f} }},\n'
                )
            code += (
                "}\n\n"
                "-- em OnMainProc, logo apos RenderImage do botao de fechar:\n"
                "for _, el in ipairs(Panel9Demo_Elements) do\n"
                "\tRenderImage(el.tex, x + el.x, y + el.y, el.w, el.h)\n"
                "end\n\n"
                "-- em OnClickEvent, antes do bloco \"Block click-through\":\n"
                "for _, el in ipairs(Panel9Demo_Elements) do\n"
                "\tlocal ex, ey = x + el.x, y + el.y\n"
                "\tif mx >= ex and mx <= ex + el.w and my >= ey and my <= ey + el.h then\n"
                "\t\tConsumeClick()\n"
                "\t\t-- TODO: acao do botao\n"
                "\t\treturn\n"
                "\tend\n"
                "end"
            )

        self.code_text.delete("1.0", "end")
        self.code_text.insert("1.0", code)

    # ---- entry field edits ------------------------------------------------

    def on_entry_change(self, *_):
        try:
            w = max(60, min(640, self.var_w.get()))
            h = max(60, min(480, self.var_h.get()))
            cs = max(10, min(80, self.var_close.get()))
        except tk.TclError:
            return  # mid-edit, not a valid int yet
        self.panel_w, self.panel_h, self.close_size = w, h, cs
        self.redraw()

    # ---- dragging (resize handle / close button, inside the main canvas) --

    def on_press(self, event):
        hx_min, hy_min, hx_max, hy_max = self._handle_bounds()
        if hx_min - 12 <= event.x <= hx_max + 12 and hy_min - 12 <= event.y <= hy_max + 12:
            self.drag = "resize"
            return

        cx, cy, cw, ch = self.close_rect()
        cpx, cpy = cx * SCALE, cy * SCALE
        cpw, cph = cw * SCALE, ch * SCALE
        if cpx <= event.x <= cpx + cpw and cpy <= event.y <= cpy + cph:
            self.drag = "close"
            return

        # Selected element's own resize handle - checked before the general
        # element-body hit test below, same "handle wins over body" priority
        # the panel's own handle vs. close-button check already has.
        selected = self._find_element(self.selected_element_id)
        if selected is not None:
            px, py, _, _ = self.panel_rect()
            ex = (px + selected["x"] + selected["w"]) * SCALE
            ey = (py + selected["y"] + selected["h"]) * SCALE
            if ex - 10 <= event.x <= ex + 10 and ey - 10 <= event.y <= ey + 10:
                self.drag = "element_resize"
                return

        # Element bodies, topmost (most recently added) first, so an
        # overlapping element on top is the one that gets grabbed.
        px, py, _, _ = self.panel_rect()
        for el in reversed(self.elements):
            ex = (px + el["x"]) * SCALE
            ey = (py + el["y"]) * SCALE
            ew = el["w"] * SCALE
            eh = el["h"] * SCALE
            if ex <= event.x <= ex + ew and ey <= event.y <= ey + eh:
                self.selected_element_id = el["id"]
                self._sync_element_list_selection()
                self._element_drag_offset = (event.x / SCALE - (px + el["x"]), event.y / SCALE - (py + el["y"]))
                self.drag = "element_move"
                self.redraw()
                return

        # Clicked empty panel space (or outside it entirely) - deselect
        # whatever element was active, if any.
        if self.selected_element_id is not None:
            self.selected_element_id = None
            self.element_list.selection_clear(0, "end")
            self.redraw()

        self.drag = None

    def _find_element(self, element_id):
        for el in self.elements:
            if el["id"] == element_id:
                return el
        return None

    def _sync_element_list_selection(self):
        for i, el in enumerate(self.elements):
            if el["id"] == self.selected_element_id:
                self.element_list.selection_clear(0, "end")
                self.element_list.selection_set(i)
                self.element_list.see(i)
                return

    def _handle_bounds(self):
        x, y, w, h = self.panel_rect()
        hx, hy = (x + w) * SCALE, (y + h) * SCALE
        return hx, hy, hx, hy

    def on_drag(self, event):
        if self.drag == "resize":
            # The panel is always centered (DrawCentered()), so its own
            # left/top edge is itself a function of width/height
            # ("x = (LOGICAL_W - w) / 2") - the previous version computed
            # new_w/new_h as "mouse position minus THIS frame's x/y", but
            # that x/y had already shifted from the *previous* frame's
            # resize, so each motion event compounded on a stale reference
            # instead of the fixed screen center - width/height grew (or
            # shrank) at roughly double the rate the mouse actually moved,
            # compounding every frame, which reads as "stuck"/erratic
            # dragging rather than a clean 1:1 follow. Solving directly for
            # w/h from the handle's real screen-space relationship to the
            # canvas center fixes both axes at once (this was never actually
            # width-specific vs height-specific - it distorts both, evenly).
            mouse_x = event.x / SCALE
            mouse_y = event.y / SCALE
            new_w = 2 * (mouse_x - LOGICAL_W / 2)
            new_h = 2 * (mouse_y - LOGICAL_H / 2)
            self.panel_w = max(60, min(640, new_w))
            self.panel_h = max(60, min(480, new_h))
            self.redraw()
        elif self.drag == "close":
            x, y, w, h = self.panel_rect()
            px_logical = event.x / SCALE
            py_logical = event.y / SCALE
            off_x = (x + w - px_logical) / self.close_size
            off_y = (y - py_logical) / self.close_size
            self.off_x = max(-1.0, min(2.0, off_x))
            self.off_y = max(-1.0, min(2.0, off_y))
            self.redraw()
        elif self.drag == "element_move":
            el = self._find_element(self.selected_element_id)
            if el is not None:
                px, py, _, _ = self.panel_rect()
                offset_x, offset_y = self._element_drag_offset
                el["x"] = event.x / SCALE - offset_x - px
                el["y"] = event.y / SCALE - offset_y - py
                self.redraw()
        elif self.drag == "element_resize":
            el = self._find_element(self.selected_element_id)
            if el is not None:
                px, py, _, _ = self.panel_rect()
                # Elements aren't centered like the panel is - they're
                # positioned by a fixed top-left corner - so unlike the
                # panel's own resize this is a plain "mouse minus origin",
                # no centering correction needed.
                new_w = event.x / SCALE - (px + el["x"])
                new_h = event.y / SCALE - (py + el["y"])
                el["w"] = max(8, min(600, new_w))
                el["h"] = max(8, min(460, new_h))
                self.redraw()

    def on_release(self, _event):
        if self.drag == "resize":
            self.push_history(f"Redimensionou painel: {int(self.panel_w)}x{int(self.panel_h)}")
        elif self.drag == "close":
            self.push_history("Moveu botao de fechar")
        elif self.drag in ("element_move", "element_resize"):
            el = self._find_element(self.selected_element_id)
            if el is not None:
                verb = "Moveu" if self.drag == "element_move" else "Redimensionou"
                self.push_history(f"{verb} botao: {el['name']}")
        self.drag = None

    # ---- element list (sidebar) ------------------------------------------

    def on_element_list_select(self, _event):
        sel = self.element_list.curselection()
        if not sel or sel[0] >= len(self.elements):
            return
        self.selected_element_id = self.elements[sel[0]]["id"]
        self.redraw()

    def remove_selected_element(self):
        sel = self.element_list.curselection()
        if not sel or sel[0] >= len(self.elements):
            return
        index = sel[0]
        removed = self.elements.pop(index)
        self.element_list.delete(index)
        if self.selected_element_id == removed["id"]:
            self.selected_element_id = None
        self.push_history(f"Botao removido: {removed['name']}")
        self.redraw()

    # ---- dragging FROM the library onto the canvas ------------------------

    def _lib_drag_start(self, name, event):
        self.lib_drag_name = name

        float_win = tk.Toplevel(self.root)
        float_win.overrideredirect(True)
        float_win.attributes("-topmost", True)
        try:
            float_win.attributes("-alpha", 0.85)
        except tk.TclError:
            pass  # per-pixel alpha not available on every platform - fine without it
        lbl = tk.Label(float_win, image=self.lib_thumbs[name], bg=self.ACCENT, bd=2, relief="solid")
        lbl.pack()
        float_win.geometry(f"+{event.x_root - THUMB_SIZE // 2}+{event.y_root - THUMB_SIZE // 2}")
        self.lib_drag_float = float_win

        # Tkinter routes subsequent Motion/ButtonRelease to the widget that
        # received the ButtonPress (implicit grab) - binding here, on the
        # library canvas, is enough to keep tracking the drag even once the
        # cursor leaves this widget's own bounds.
        self.lib_canvas.bind("<B1-Motion>", self._lib_drag_motion)
        self.lib_canvas.bind("<ButtonRelease-1>", self._lib_drag_release)

    def _lib_drag_motion(self, event):
        if self.lib_drag_float is not None:
            self.lib_drag_float.geometry(f"+{event.x_root - THUMB_SIZE // 2}+{event.y_root - THUMB_SIZE // 2}")

    def _lib_drag_release(self, event):
        self.lib_canvas.unbind("<B1-Motion>")
        self.lib_canvas.unbind("<ButtonRelease-1>")

        if self.lib_drag_float is not None:
            self.lib_drag_float.destroy()
            self.lib_drag_float = None

        name = self.lib_drag_name
        self.lib_drag_name = None
        if name is None:
            return

        target = self._hit_test_screen_point(event.x_root, event.y_root)
        if target is None:
            return  # dropped somewhere that isn't the panel or the close button - no-op

        try:
            raw_img = load_mu_image(os.path.join(DATA_INTERFACE, name))
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror("Erro ao carregar", f"{name}: {exc}")
            return

        native_w, native_h = raw_img.size
        img = _cap_size(raw_img)

        if target == "bg":
            self.stone_pil = img
            # Fit the panel to the dropped image's own real size instead of
            # stretching it into whatever size the panel already was -
            # Evan's report: dropping an asset stretched/distorted it to
            # fill the old dimensions instead of showing at its own size.
            self.panel_w = max(60, min(640, native_w))
            self.panel_h = max(60, min(480, native_h))
            self.status_label.config(
                text=f"Fundo do painel trocado para {name} ({native_w}x{native_h}).", fg=self.ACCENT)
            self.push_history(f"Fundo: {name}")
        elif target == "close":
            self.close_pil = img
            # Close button only has one size (assumed square) - use the
            # larger side so a non-square drop still fits without cropping.
            self.close_size = max(10, min(80, max(native_w, native_h)))
            self.status_label.config(
                text=f"Botao de fechar trocado para {name} ({native_w}x{native_h}).", fg=self.ACCENT)
            self.push_history(f"Fechar: {name}")
        elif target == "panel":
            # Dropping onto the panel body itself now ADDS a new clickable
            # element there instead of swapping the background - the "Fundo"
            # swatch above owns background-swapping now, see this method's
            # target=="bg" branch and _hit_test_screen_point()'s comment.
            cx0 = self.canvas.winfo_rootx()
            cy0 = self.canvas.winfo_rooty()
            drop_x_logical = (event.x_root - cx0) / SCALE
            drop_y_logical = (event.y_root - cy0) / SCALE

            panel_x, panel_y, _, _ = self.panel_rect()
            el_w = min(native_w, 160)   # keep a first-drop element to a sane
            el_h = min(native_h, 160)   # on-panel size even for a huge asset
            # Center the new element on the drop point, stored relative to
            # the panel's own top-left so it stays put if the panel is
            # resized/recentered afterward.
            el_x = drop_x_logical - panel_x - el_w / 2
            el_y = drop_y_logical - panel_y - el_h / 2

            element = {
                "id": self.next_element_id,
                "name": name,
                "pil": img,
                "x": el_x, "y": el_y, "w": el_w, "h": el_h,
            }
            self.next_element_id += 1
            self.elements.append(element)
            self.selected_element_id = element["id"]
            self.element_list.insert("end", name)
            self.element_list.selection_clear(0, "end")
            self.element_list.selection_set("end")

            self.status_label.config(text=f"Botao criado: {name}. Arraste pra mover, puxe o canto pra redimensionar.",
                                      fg=self.ACCENT)
            self.push_history(f"Botao adicionado: {name}")

        self.redraw()

    def _hit_test_screen_point(self, x_root, y_root):
        """Which drop target (if any) a screen-space point lands on. Checks
        the sidebar's "Fundo" swatch first (a separate small widget, not on
        the main canvas at all), then falls back to canvas-local logical
        coordinates for everything else - same rects on_press() already
        uses, just fed a point that came from a different widget's event."""
        bx0 = self.bg_swatch.winfo_rootx()
        by0 = self.bg_swatch.winfo_rooty()
        if bx0 <= x_root <= bx0 + self.bg_swatch.winfo_width() and by0 <= y_root <= by0 + self.bg_swatch.winfo_height():
            return "bg"

        cx0 = self.canvas.winfo_rootx()
        cy0 = self.canvas.winfo_rooty()
        local_x = x_root - cx0
        local_y = y_root - cy0

        if not (0 <= local_x <= self.canvas.winfo_width() and 0 <= local_y <= self.canvas.winfo_height()):
            return None

        # The close button deliberately overlaps the panel's top-right
        # corner (off_x/off_y default to 0.7/0.3 - see close_rect()'s own
        # comment) so it reads as "sitting on" the frame, same as the
        # exemplo.jpg reference. That's fine for rendering, but a *drop*
        # hit-test using the button's full bounding box means any drop
        # anywhere in that overlap zone gets misread as targeting the
        # close button instead of the panel underneath it - reported by
        # Evan as "toda imagem vem com o icone X" (every dropped image
        # ends up replacing the close icon instead of the panel). Shrunk
        # to a centered 60% of the button's own box for drop purposes only
        # (on_press()'s drag-the-button-itself hit-test is untouched - a
        # deliberate direct click on the button doesn't have this problem).
        cx, cy, cw, ch = self.close_rect()
        inset_w, inset_h = cw * 0.2, ch * 0.2
        cpx, cpy = (cx + inset_w) * SCALE, (cy + inset_h) * SCALE
        cpw, cph = (cw - inset_w * 2) * SCALE, (ch - inset_h * 2) * SCALE
        if cpx <= local_x <= cpx + cpw and cpy <= local_y <= cpy + cph:
            return "close"

        x, y, w, h = self.panel_rect()
        px, py, pw, ph = x * SCALE, y * SCALE, w * SCALE, h * SCALE
        if px <= local_x <= px + pw and py <= local_y <= py + ph:
            return "panel"

        return None

    # ---- history / "timeline" --------------------------------------------
    #
    # A snapshot per meaningful change (library drop, or a finished resize/
    # close-drag - not every intermediate frame, and not every keystroke in
    # the number fields, or this would be nothing but noise). Click an entry
    # to jump back to it; "Remover selecionado" deletes an entry from the
    # list without necessarily changing the current live state - lets Evan
    # prune a bad drop out of the list, per his own "consigo deletar o que
    # quero" request, distinct from just restoring an older state.

    def push_history(self, label):
        state = {
            "stone_pil": self.stone_pil,
            "close_pil": self.close_pil,
            "panel_w": self.panel_w,
            "panel_h": self.panel_h,
            "close_size": self.close_size,
            "off_x": self.off_x,
            "off_y": self.off_y,
            # Elements' x/y/w/h are mutated IN PLACE while dragging (see
            # on_drag()'s "element_move"/"element_resize" branches) - a
            # shallow dict-copy per element means this snapshot keeps its
            # own values even after a later drag changes the live ones
            # (the PIL image inside each copy is still shared by reference,
            # which is fine - it's never mutated, only ever replaced).
            "elements": [dict(el) for el in self.elements],
            "selected_element_id": self.selected_element_id,
            "next_element_id": self.next_element_id,
        }
        self.history.append({"label": label, "state": state})
        self.history_list.insert("end", label)
        self.history_list.see("end")

    def restore_history(self, index):
        state = self.history[index]["state"]
        self.stone_pil = state["stone_pil"]
        self.close_pil = state["close_pil"]
        self.panel_w = state["panel_w"]
        self.panel_h = state["panel_h"]
        self.close_size = state["close_size"]
        self.off_x = state["off_x"]
        self.off_y = state["off_y"]
        # Copy again on the way OUT of history too, so dragging an element
        # after restoring doesn't reach back and mutate this same snapshot.
        self.elements = [dict(el) for el in state["elements"]]
        self.selected_element_id = state["selected_element_id"]
        self.next_element_id = state["next_element_id"]

        self.element_list.delete(0, "end")
        for el in self.elements:
            self.element_list.insert("end", el["name"])
        self._sync_element_list_selection()

        self.redraw()

    def on_history_select(self, _event):
        sel = self.history_list.curselection()
        if not sel:
            return
        self.restore_history(sel[0])
        self.status_label.config(text=f"Voltou para: {self.history[sel[0]]['label']}", fg=self.ACCENT)

    def remove_history_selected(self):
        sel = self.history_list.curselection()
        if not sel:
            return
        index = sel[0]
        if len(self.history) <= 1:
            self.status_label.config(text="Nao da pra remover o unico item do historico.", fg=self.DIM)
            return
        del self.history[index]
        self.history_list.delete(index)

    # ---- actions --------------------------------------------------------

    def copy_code(self):
        self.root.clipboard_clear()
        self.root.clipboard_append(self.code_text.get("1.0", "end-1c"))
        self.status_label.config(text="Codigo copiado - cole no Panel9Demo.lua.", fg="#e8c778")

    def save_and_deploy(self):
        try:
            patch_lua_file(int(self.panel_w), int(self.panel_h), int(self.close_size),
                            self.off_x, self.off_y)
            out = deploy()
            self.status_label.config(
                text="Salvo e implantado. Feche e abra o Main.exe pra testar.\n" + out.strip(),
                fg="#9fc98a")
        except Exception as exc:  # noqa: BLE001 - shown to the user either way
            messagebox.showerror("Erro ao salvar", str(exc))
            self.status_label.config(text="Falhou - nada foi escrito.", fg="#d98a72")


def main():
    if not os.path.isfile(STONE_PATH):
        print(f"Nao encontrei {STONE_PATH} - rode este script de dentro do projeto (Tools\\PanelDesigner\\).")
        sys.exit(1)

    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
