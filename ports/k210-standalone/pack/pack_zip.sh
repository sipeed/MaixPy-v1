#!/bin/bash
cd bin_file
zip -r ./firmware.zip ./* 
mv firmware.zip ../sipeedm1.kfpkg
