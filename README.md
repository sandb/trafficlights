trafficlights
=============

Silly c program to steer traffic lights connected to parallel port based on status of a jenkins job/plan

Usage
-----

Usage: trafficlights [OPTION...]

  -j, --job=jobname          The name of the job to monitor
  -r, --refreshrate=seconds  After how many seconds do we update the status of
                             a job
  -s, --server=server url    The url of the jenkins server
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Report bugs to <pieter.iserbyt@gmail.com>.

Example
-------

The following starts trafficlights to monitor the jenkins server for the status of a job/plan called "the one that frequently fails".

	$ sudo trafficlights -s http://host.sx:8080/jenkins -j "the one that frequently fails" -r 20

You have to run it as root, because you need to access the parallel port.

Trafficlights adds the suffix `api/json` to your url to get the json status overview of all jobs on the server, and then searches for the specified jobname to get its status.

One would run it as a service, or just as a long running program, e.g. in a screen session or so. 

Once you kill it, the lights stay they way they were set last, so you can't detect if it is still running by looking at the lights.

Trafficlights is hardwired to access the first parallel port, at address `0x378`, using device `/dev/parport0`. If you want to change this, you'll have to edit the code.

Lights
------

The lights we got are based on http://www.unlimitednovelty.com/2010/10/how-to-make-cheap-ci-traffic-light.html, for which many thanks and credits.

The light codes trafficlights uses is the same as on the jenkins website:

* green steady: last builds was succesful, not building now
* green flashing: last build was succesful, new build in progress
* orange steady: last build was unstable, not building now
* orange flashing: last build was unstable, new build in progress
* red steady: last build failed, not building now
* red flashing: last build failed, new build in progress

and then one extra:

* all lights flashing: jenkins server does not know specified job name.

Dependencies
------------

Trafficlights uses:

* json-c 0.10, by Eric Haszlakiewicz, MIT License, see: <https://github.com/json-c/json-c/wiki>
* libcurl 7.30.0, by Daniel Stenberg, MIT/X derivate license, see <http://curl.haxx.se/libcurl/>

Building
--------

A simple `make` will get you the binary.

