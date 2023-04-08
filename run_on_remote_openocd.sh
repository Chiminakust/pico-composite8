#!/bin/sh

src/openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -s tcl -c "bindto 0.0.0.0"
