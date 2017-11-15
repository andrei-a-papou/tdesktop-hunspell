# Based on https://aur.archlinux.org/packages/telegram-desktop

pkgname=telegram-desktop
pkgver=1.1.23
pkgrel=1
pkgdesc='Official desktop version of Telegram messaging app.'
arch=('i686' 'x86_64')
url="https://desktop.telegram.org/"
license=('GPL3')
depends=(
    'ffmpeg'
    'icu'
    'jasper'
    'libmng'
    'libxkbcommon-x11'
    'libinput'
    'libproxy'
    'openal'
    'tslib'
    'xcb-util-wm'
    'xcb-util-keysyms'
    'xcb-util-image'
    'xcb-util-renderutil'
    'hicolor-icon-theme'
    'openssl-1.0'
)
makedepends=(
    'git'
    'libappindicator-gtk2'
    'libva'
    'mtdev'
    'libexif'
    'libwebp'
    'google-breakpad-git'
    'chrpath'
    'cmake'
    'python'
    'python2'

    # QT5 build dependencies
    'xcb-util-keysyms'
    'libgl'
    'fontconfig'
    'xcb-util-wm'
    'libxrender'
    'libxi'
    'sqlite'
    'xcb-util-image'
    'harfbuzz-icu'
    'tslib'
    'libinput'
    'libsm'
    'libxkbcommon-x11'
    # For qtimageformats
    'libjpeg-turbo'
    'libpng'
    'libtiff'
    'libmng'
    'libwebp'
)
qt_version=5.6.2
source=(
    "tdesktop::git+https://github.com/telegramdesktop/tdesktop.git#tag=v$pkgver"
    "GSL::git+https://github.com/Microsoft/GSL.git"
    "variant::git+https://github.com/mapbox/variant.git"
    "libtgvoip::git+https://github.com/telegramdesktop/libtgvoip.git"
    "https://download.qt.io/official_releases/qt/${qt_version%.*}/$qt_version/submodules/qtbase-opensource-src-$qt_version.tar.xz"
    "https://download.qt.io/official_releases/qt/${qt_version%.*}/$qt_version/submodules/qtimageformats-opensource-src-$qt_version.tar.xz"
    "git+https://chromium.googlesource.com/external/gyp#commit=702ac58e4772"
    "telegram-desktop.desktop"
    "tg.protocol"
    "aur-fixes.diff"
    "libtgvoip-fixes.diff"
    "hunspell-telegram.diff"
    "Hunspell.cpp"
)
sha256sums=(
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
    'SKIP'
)

prepare() {
    cd "$srcdir/tdesktop"

    mkdir -p "$srcdir/Libraries"

    local qt_patch_file="$srcdir/tdesktop/Telegram/Patches/qtbase_${qt_version//./_}.diff"
    local qt_src_dir="$srcdir/Libraries/qt${qt_version//./_}"
    if [ "$qt_patch_file" -nt "$qt_src_dir" ]; then
        rm -rf "$qt_src_dir"
        mkdir "$qt_src_dir"

        mv "$srcdir/qtbase-opensource-src-$qt_version" "$qt_src_dir/qtbase"
        mv "$srcdir/qtimageformats-opensource-src-$qt_version" "$qt_src_dir/qtimageformats"

        cd "$qt_src_dir/qtbase"
        patch -p1 -i "$qt_patch_file"

        echo "INCLUDEPATH += /usr/include/openssl-1.0" >> "$qt_src_dir/qtbase/src/network/network.pro"
    fi

    cd "$srcdir/gyp"
    git apply "$srcdir/tdesktop/Telegram/Patches/gyp.diff"
    sed -i 's/exec python /exec python2 /g' "$srcdir/gyp/gyp"

    if [ ! -h "$srcdir/Libraries/gyp" ]; then
        ln -s "$srcdir/gyp" "$srcdir/Libraries/gyp"
    fi

    cd "$srcdir/tdesktop"

    git submodule init
    git config submodule.Telegram/ThirdParty/GSL.url "$srcdir/GSL"
    git config submodule.Telegram/ThirdParty/variant.url "$srcdir/variant"
    git config submodule.Telegram/ThirdParty/libtgvoip.url "$srcdir/libtgvoip"
    git submodule update

    git apply "$srcdir/aur-fixes.diff"
    git apply "$srcdir/hunspell-telegram.diff"

    cd Telegram/ThirdParty/libtgvoip
    git checkout .
    git apply "$srcdir/libtgvoip-fixes.diff"

    sed '/#include <QTextEdit>/d' "$srcdir/Hunspell.cpp" > "$srcdir/tdesktop/Telegram/SourceFiles/chat_helpers/Hunspell.cpp"
}

build() {
    # Build patched Qt
    local qt_src_dir="$srcdir/Libraries/qt${qt_version//./_}"

    export OPENSSL_LIBS='-L/usr/lib/openssl-1.0 -lssl -lcrypto'
    cd "$qt_src_dir/qtbase"
    ./configure \
        -prefix "$srcdir/qt" \
        -release \
        -force-debug-info \
        -opensource \
        -confirm-license \
        -system-zlib \
        -system-libpng \
        -system-libjpeg \
        -system-freetype \
        -system-harfbuzz \
        -system-pcre \
        -system-xcb \
        -system-xkbcommon-x11 \
        -no-gtkstyle \
        -static \
        -nomake examples \
        -nomake tests \
        -no-opengl
    make
    make install
    export PATH="$srcdir/qt/bin:$PATH"

    cd "$qt_src_dir/qtimageformats"
    qmake .
    make
    make install

    # Build Telegram Desktop
    rm -rf "$srcdir/tdesktop/out"
    cd "$srcdir/tdesktop/Telegram/gyp"

    "$srcdir/Libraries/gyp/gyp" \
        -Dlinux_path_qt="$srcdir/qt" \
        -Dlinux_lib_ssl='-L/usr/lib/openssl-1.0 -lssl' \
        -Dlinux_lib_crypto='-L/usr/lib/openssl-1.0 -lcrypto' \
        -Dlinux_lib_icu="-licuuc -licutu -licui18n" \
        -Dlinux_path_opus_include="/usr/include/opus" \
        --depth=. --generator-output=../.. -Goutput_dir=out Telegram.gyp --format=cmake
    cd "$srcdir/tdesktop/out/Release"
    cmake .
    make
    chrpath --delete Telegram
}

package() {
    install -dm755 "$pkgdir/usr/bin"
    install -m755 "$srcdir/tdesktop/out/Release/Telegram" "$pkgdir/usr/bin/telegram-desktop"

    install -d "$pkgdir/usr/share/applications"
    install -m644 "$srcdir/telegram-desktop.desktop" "$pkgdir/usr/share/applications/telegram-desktop.desktop"

    install -d "$pkgdir/usr/share/kde4/services"
    install -m644 "$srcdir/tg.protocol" "$pkgdir/usr/share/kde4/services/tg.protocol"

    local icon_size icon_dir
    for icon_size in 16 32 48 64 128 256 512; do
        icon_dir="$pkgdir/usr/share/icons/hicolor/${icon_size}x${icon_size}/apps"

        install -d "$icon_dir"
        install -m644 "$srcdir/tdesktop/Telegram/Resources/art/icon${icon_size}.png" "$icon_dir/telegram-desktop.png"
    done
}
