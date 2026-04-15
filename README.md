# configurable, lightweight, piece-table based text editor for linux written in c++ using qt6 library

This editor opens large files blazingly fast (10GB file uses **110MB** of RAM only).

Other text editors:
- Neovim: 6044,41MB
- GNU Emacs: 5234,25MB

---
c++ compiler and qt6 development libraries:

**ubuntu / debian / mint / pop!_os:**
```bash
sudo apt update
sudo apt install build-essential qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
```

**fedora / red hat / almalinux:**
```bash
sudo dnf install gcc-c++ make qt6-qtbase-devel
```

**arch / manjaro / endeavouros:**
```bash
sudo pacman -S base-devel qt6-base
```

### full example for fedora
```bash
sudo dnf install -y gcc-c++ make qt6-qtbase-devel && \
qmake-qt6 qt-editor.pro && \
make clean && \
make -j$(nproc) && \
./qt-editor && \
sudo cp qt-editor /usr/local/bin/
```

* **``` ctrl + ` ```** (backtick): toggle built-in linux console
*   `ctrl + f` to search
*   **macros:** 
    *   `Ctrl + Shift + R` to start recording your actions.
    *   `Ctrl + Shift + T` to stop and save.
    *   `Ctrl + Shift + P` to open the Macro Manager and play them back.
*   `ctrl + k` to open files recently opened files.



