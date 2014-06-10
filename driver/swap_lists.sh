#!/bin/sh

mv mask_images_pinhole.txt a
mv screen_images_pinhole.txt mask_images_pinhole.txt
mv a screen_images_pinhole.txt

mv mask_images_NMF.txt a
mv screen_images_pinhole.txt mask_images_NMF.txt
mv a screen_images_NMF.txt
