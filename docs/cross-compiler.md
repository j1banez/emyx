# Cross Compiler

https://wiki.osdev.org/GCC_Cross-Compiler

## 0. Goal
Build a _freestanding_ cross-compiler:

- **Host:** your current OS
- **Target:** `i686-elf`

Result: a `i686-elf-gcc` that **does not use your host’s libc/headers**.

## 1. Install dependencies
On a Debian/Ubuntu-like system

```sh
sudo apt update
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo
```

## 2. Set paths & target
Pick where to install the cross-compiler

```sh
export PREFIX="$HOME/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
```

## 3. Download sources
Create a source directory and grab **Binutils** and **GCC**
- https://ftp.gnu.org/gnu/binutils/
- https://ftp.gnu.org/gnu/gcc/

```sh
mkdir -p $HOME/src
cd $HOME/src 
# Download from GNU mirrors (example with placeholder versions x.y.z)
# Replace x.y.z with actual version numbers, e.g. binutils-2.42, gcc-14.2.0
wget https://ftp.gnu.org/gnu/binutils/binutils-x.y.z.tar.xz 
wget https://ftp.gnu.org/gnu/gcc/gcc-x.y.z/gcc-x.y.z.tar.xz
tar xf binutils-x.y.z.tar.xz
tar xf gcc-x.y.z.tar.xz
```

## 4. Build & install Binutils
Build in a separate build directory (out-of-tree build):

```sh
cd $HOME/src
mkdir build-binutils
cd build-binutils
../binutils-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install
```

After this, you should have tools like `i686-elf-as`, `i686-elf-ld`, etc. in `$PREFIX/bin`.

```sh
which $TARGET-as
```

## 5. Build & install GCC (freestanding)
Now build a minimal cross GCC that doesn’t depend on any target libc:

```sh
cd $HOME/src
mkdir build-gcc
cd build-gcc
../gcc-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers
# Build the compiler itself
make all-gcc 
# Build libgcc for the target (freestanding)
make all-target-libgcc
# Install
make install-gcc
make install-target-libgcc
```

After this, you should have `i686-elf-gcc` in `$PREFIX/bin`.

```sh
which $TARGET-gcc
```

---

Remember: this compiler is **freestanding**:
- It does _not_ use your host system libc/headers by default
- It can include target headers you provide (for example via a sysroot)
- It _is_ enough to compile your kernel and bare-metal code
