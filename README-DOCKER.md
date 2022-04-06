# Libabigail Docker

The [Dockerfile](Dockerfile) provided alongside the code provides a base container
to build libabigail. We will build and deploy a container on:

 - releases
 - pushes to the main (master) branch

### Usage

Here is how to build the container. Note that we build so it belongs to the same
namespace as the repository here. "ghcr.io" means "GitHub Container Registry" and
is the [GitHub packages](https://github.com/features/packages) registry that supports
 Docker images and other OCI artifacts.

```bash
$ docker build -t ghcr.io/woodard/libabigail .
```

Note that we use `ghcr.io/rse-ops/gcc-ubuntu-22.04:gcc-10.3.0` as the base image. If it's
ever desired to build libabigail with different compilers or OS (across a matrix) we can do that too :)

### Shell

To shell into the container:

```bash
$ docker run -it ghcr.io/woodard/libabigail bash
```

Off the bat, you can find the abi executables:


```bash
# which abidiff
/opt/abigail-env/.spack-env/view/bin/abidiff
```

That also means you can go to the environment in `/opt/abigail-env` and (given you
have the source code bound to `/src`) build and test again.

```bash
$ spack install
```

And that's it! This workflow should make it easy to install development versions of libabigail with spack.
We will also use the "production" containers to grab libraries in:

```
$ ls /opt/abigail-env/.spack-env/view/
bin  include  lib  share
```
To run the libabigail action on (yes, very meta!)
