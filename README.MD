
Http client and server software library in C

* Copyright, Peter Van Oudenaren
*  EBS Inc. 1987-2015
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.

* contact sales@ebsembeddedsoftware.com


This project provides APIs and examples that allow you to add HTTP client and server code to embedded applications.

A more elaborate description of what is included may be had by reading the file names ReadMe.pdf in the root of this repo.

Known issues -
  ssl is incorporated but there are some problems that need to be debugged.
  client code was extracted from a UPnP implementation, so its core is pretty robust but the examples using its standalone API and some of the API elements are not super mature.
  A Visual 'C' 2010 project is provided in the folder: .\httpserverandclient\http\v0.2.6\project\msvs.net2010\win32\serverdemo, but a Linux project needs to be developed and debugged.
  The source code already supports Linux, Make file need to be created and the code needs to be tested.


