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
                                                                        
// $Revision: 1.5 $
// $Date: 2002-10-03 18:52:03 $
// $Source: /usr/local/cvs/OpenSees/SRC/material/section/ElasticSection2d.h,v $
                                                                        
                                                                        
///////////////////////////////////////////////////////
// File:  ~/Src/element/hinge/ElasticSection2d.h
//
// Written by Matthew Peavy
//
// Written:  Feb 13, 2000
// Debugged: Feb 14, 2000
// Revised:  May 2000 -- MHS
//
//
// Purpose:  This header file contains the prototype
// for the ElasticSection2d class.

#ifndef ElasticSection2d_h
#define ElasticSection2d_h

#include <SectionForceDeformation.h>
#include <Matrix.h>
#include <Vector.h>

class Channel;
class FEM_ObjectBroker;
class Information;

class ElasticSection2d: public SectionForceDeformation
{
  public:
	ElasticSection2d (int tag, double E, double A, double I);
    ElasticSection2d (int tag, double EA, double EI);
    ElasticSection2d (void);    
	~ElasticSection2d (void);

    int commitState (void);
    int revertToLastCommit (void);
    int revertToStart (void);

    int setTrialSectionDeformation (const Vector&);
    const Vector &getSectionDeformation (void);

    const Vector &getStressResultant (void);
    const Matrix &getSectionTangent (void);
    const Matrix &getInitialTangent (void);
    const Matrix &getSectionFlexibility (void);
    const Matrix &getInitialFlexibility(void);

    SectionForceDeformation *getCopy (void);
    const ID &getType (void);
    int getOrder (void) const;
    
    int sendSelf (int commitTag, Channel &theChannel);
    int recvSelf (int commitTag, Channel &theChannel,
		  FEM_ObjectBroker &theBroker);
    
    void Print (ostream &s, int flag =0);

  protected:

  private:
   
    double E, A, I;

    Vector e;			// section trial deformations
    Vector eCommit;
    
    static Vector s;
    static Matrix ks;
    static ID code;
};

#endif
