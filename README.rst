vmod_header
===========

Varnish Module (vmod) for manipulation of duplicated headers (for instance
multiple set-cookie headers).

Installation
============

Installation requires the Varnish source tree (only the source matching the
binary installation).

1. ./autogen.sh  (for git-installation)
2. ./configure VARNISHSRC=/path/to/your/varnish/source/varnish-cache
3. make
4. make install (may require root: sudo make install)

VARNISHSRCDIR is the directory of the Varnish source tree for which to
compile your vmod. Both the VARNISHSRCDIR and VARNISHSRCDIR/include
will be added to the include search paths for your module.

Optionally you can also set the vmod install dir by adding VMODDIR=DIR
(defaults to the pkg-config discovered directory from your Varnish
installation).

Usage
=====

Example VCL::

	backend foo { ... };

	import header;

	sub vcl_fetch {
		header.append(beresp.http.Set-Cookie,"foo=bar");
	}

Appending
---------

To append an additional header identical to a previous header, use
header.append(<the header>, <content>);

Retrieving
----------

You can fetch the value of the first matching header using get(). Example::

        sub vcl_fetch {
                set beresp.http.x-test = header.get(beresp.http.Set-Cookie,"user=");
        }

Acknowledgements
================

Author: Kristian Lyngstøl <kristian@bohemians.org>, Varnish Software AS
Skeleton by Martin Blix Grydeland <martin@varnish-software.com>, vmods are
part of Varnish Cache 3.0 and beyond.

TODO
====

- Removing headers
- Copying headers
- Altering headers
- Why is vrt_selecthttp static in cache_vrt.c?
  Should at least be re-named locally.


