/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
                                                                        
// $Revision: 1.2 $
// $Date: 2001-05-16 04:19:11 $
// $Source: /usr/local/cvs/OpenSees/SRC/recorder/FilePlotter.h,v $
                                                                        
                                                                        
// File: ~/recorder/tcl/FilePlotter.h.h
// 
// Written: fmk 
// Created: 11/99
// Revision: A
//
// Description: This file contains the class definition for FilePlotter.
// A FilePlotter will create a line graph using xy points found in a file.

//
// What: "@(#) ModelBuilder.h, revA"

#ifndef FilePlotter_h
#define FilePlotter_h

#include <Recorder.h>
#include <G3Globals.h>

class Renderer;
class ColorMap;
class ID;

class FilePlotter : public Recorder
{
  public:
    FilePlotter(char *fileName,
		char *windowTitle, 
		int xLoc, int yLoc, int width, int height);
    
    ~FilePlotter();    

    int plotFile();

    int record(int cTag);
    int playback(int cTag);
    void restart(void);    

    int setFile(char *newFile);
    int setCol(const ID &theCols);

  protected:

  private:
    ColorMap *theMap;
    Renderer *theRenderer;
    ID *cols;
    char fileName[MAX_FILENAMELENGTH];
};

#endif







