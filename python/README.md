# Libabigail Python

Libabigail Python is a simple set of Python wrappers to expose some of Libabigails's
key functionality in Python.

## Getting Started

### 1. Development Environment

If you like to keep development projects separate and do not want to compile
libabigail on your machine, we provide a [Dockerfile](Dockerfile) to build
a container instead. You can [install Docker](https://docs.docker.com/get-docker/)
for your platform of choice, and then build the container as follows:

```bash
# build abigail with file Dockerfile with context ../
$ docker build -t abigail -f Dockerfile ../
```

In the above, we are setting the build context to the parent directory (with 
libabigail) so that we can compile and install it.
When the container is finished, if you are developing interactivly,
you'll want to bind the code directory (to write files and work interactively):

```bash
$ docker run -it --rm -v ../:/code/ abigail bash
```

from the root of the repository you can also do:

```bash
docker run -it --rm -v $PWD:/code/ abigail bash
```

Otherwise, for a completely isolated environment you can do:

```bash
$ docker run -it --rm abigail bash
```

Note that development of [llnl/shroud](https://github.com/llnl/shroud) is quite active, so it could be common
that something works, and if you rebuild the container with an updated develop branch,
something is broken. We install from the develop branch so we have the latest, but
when there is something more production ready we should choose a version.

### 2. Compiling

Once you are in the container (or if you've followed the instructions in
[COMPILING](../COMPILING) to install libabigail on your host)
you can then also compile the python bindings. There are two steps, and you
may need to do one or the other, or both:

 1. Re-generate the shroud output if any of your cpp code has changed (meaning function signatures, additions of variables to [libabigail.yaml](liabigail.yaml), etc.
 2. Just compile Python bindings if you just change internal content of functions.

You should compile libabigail as you [normally would](COMPILING) but add the flag
to `--enable-python-bindings` (this starts from the repository root):

```bash
$ autoreconf -i
$ mkdir build
$ cd build
$ ../configure --prefix=/usr/local --enable-python-bindings
$ make all install
```

You can then install the Python bindings using the generated setup.py
(note that if it already exists it will not be overwritten).

**Important: this isn't working yet, I don't know how to get the files from
the python folder added to the build's python folder!**

```bash
$ python3 setup.py install
$ python3 setup.py build # this also works without installing
```

### 3. Python Usage

Once you have your extension built and installed, you can test it out in ipython!

```python
import abipython

In [2]: parser = abipython.abipython.Libabigail()

In [3]: parser.GetVersion()
1.8.0
```

Here is an example of reading and writing a corpus to xml. You are required
to provide the path to the output file:

```python
import abipython
parser = abipython.abipython.Libabigail()

parser.ReadElfCorpusAndWriteXML("/usr/local/lib/libabigail.so", "libabigail.xml")
# Out[2]: 'libabigail.xml'
```
