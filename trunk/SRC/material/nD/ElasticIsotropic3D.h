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
                                                                        
// $Revision: 1.9 $                                                              
// $Date: 2001-07-21 20:13:13 $                                                                  
// $Source: /usr/local/cvs/OpenSees/SRC/material/nD/ElasticIsotropic3D.h,v $                                                                
                                                                        
                                                                        
#ifndef ElasticIsotropic3D_h
#define ElasticIsotropic3D_h
	 
#include <ElasticIsotropicMaterial.h>
#include <Matrix.h>
#include <Vector.h>
#include <ID.h>

#include <straint.h>
#include <stresst.h>
#include <Tensor.h>

class ElasticIsotropic3D : public ElasticIsotropicMaterial
{
  public:
    ElasticIsotropic3D (int tag, double E, double nu);
    ElasticIsotropic3D ();
    ~ElasticIsotropic3D ();

    int setTrialStrain (const Vector &v);
    int setTrialStrain (const Vector &v, const Vector &r);
    int setTrialStrainIncr (const Vector &v);
    int setTrialStrainIncr (const Vector &v, const Vector &r);
    const Matrix &getTangent (void);
    const Vector &getStress (void);
    const Vector &getStrain (void);
    
    int setTrialStrain (const Tensor &v);
    int setTrialStrain (const Tensor &v, const Tensor &r);
    int setTrialStrainIncr (const Tensor &v);
    int setTrialStrainIncr (const Tensor &v, const Tensor &r);
    const Tensor &getTangentTensor (void);
    const stresstensor getStressTensor (void);
    const straintensor getStrainTensor (void);
    //const Tensor& getStrainTensor (void);

    int commitState (void);
    int revertToLastCommit (void);
    int revertToStart (void);
    
    NDMaterial *getCopy (void);
    const char *getType (void) const;
    int getOrder (void) const;

    int sendSelf(int commitTag, Channel &theChannel);  
    int recvSelf(int commitTag, Channel &theChannel, 
    FEM_ObjectBroker &theBroker);    
    
    void Print(ostream &s, int flag =0);
    void setInitElasticStiffness(void);

  //Private functions
  private:


  protected:

  private:
    Vector sigma;		// Stress vector
    Matrix D;			// Elastic constants
    Vector epsilon;		// Strain vector

    //double exp;                 // exponent usually 0.6
    //double p_ref;               // Reference pressure, usually atmosphere pressure, i.e. 100kPa
    //double po;                  // Initial pressure of this material point
    stresstensor Stress;	// Stress tensor    
    Tensor Dt;			// Elastic constants tensor
    Tensor Dt_commit;		// last-step Elastic constants tensor
    straintensor Strain;	// Strain tensor    
	     
};


#endif


