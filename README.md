# 360D-Sampler
Takes snaps of the product/item from 360 degree.

## Overview
If data is oil, machine learning is oil refinery! 

To generate your own image data for a specific object/product/item. We have created this project, that captures 360 degree images of an object. The work is based on the image capturing details given in the [paper](https://drive.google.com/drive/folders/0B8z-ghcMsu59RjNtS1VzN1Zpb0U) 

This sampler is meant for image sampling that can be later used for training an image classifier. To get best quality and appropriate image samples the paper concludes following: 

* Azimuth - Object should be revolved 360 degree at least 24 times in equal angle. This means 360/24 = 15 degree.
* Elevation - In a circular fashion object should be captured in four verticals i.e. 90 degree / 4 = 22.5 degree.

## Repo Contains

1. Word document that gives high level details about the sampler. 
2. Mechanical Design: CAD file of the sampler design along with BOM
3. Source code to run controller and DSLR controller software to capture pictures. 

## Working Flow

Following actions and events occur during sampling: 

1. Place the object on the revolving table,
2. Enter the barcode so that the respective folder will be created with the name as ‘barcode #’
3. Follow the on screen messages to switch ON the motor and image capturing.
4. Once 360 degree movement is over, the motor stops.
5. To capture another object jump to step 1. 

## Physical Images Of The Sampler

Following images are of the physical sampler that was manufactured for demo purpose. 

![Image 1](https://www.dropbox.com/s/0mkredcyn5mxwkf/sampler-design.png?raw=true "Design")
![Image 2](https://www.dropbox.com/s/04svj0xfnbo4qdk/IMG_9475.JPG?raw=true "Img-1")
![Image 3](https://www.dropbox.com/s/tttx853wdv75rcc/IMG_9476.JPG?raw=true "Img-2")


## Software

1. Arduino: To control the motor and clicker. This code is in the ‘arduino’ directory.
2. DSLR controller: This project uses libgphotos2 to control all the DSLR mounted on the hardware chassis. This code runs on the PC/Laptop and the application code is in the following path ‘slrs/examples/sample-capture.c’ 


## Compile and Install

1. Follow the [link](https://github.com/gphoto/libgphoto2/blob/master/INSTALL) to compile the code, that run on PC.   
2. Arduino code needs to be flashed onto Aruino UNO or any of the similar version. 

