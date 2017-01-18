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
                                                                        

#include <PlaneStressLayeredMaterial.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>
#include <MaterialResponse.h>
#include <Information.h>
#include <elementAPI.h>

void* OPS_PlaneStressLayeredMaterial(void)
{
    if (OPS_GetNumRemainingInputArgs() < 4) {
	opserr << "WARNING insufficient arguments" << endln;
	opserr << "Want: nDmaterial planeStressLayeredMaterial $tag $nLayers $matTag1 $t1 ... $matTagN $nn " << endln;
	return 0;
    }
      
    int tag, nLayers, matTag;
    double h, *thickness;
    NDMaterial **theMats;

    int numdata = 1;
    if (OPS_GetIntInput(&numdata, &tag) < 0) {
	opserr << "WARNING invalid tag: nDMaterial planeStressLayeredMaterial $tag" << endln;
	return 0;
    }

    if (OPS_GetIntInput(&numdata, &nLayers) < 0) {
	opserr << "WARNING invalid nLayers" << endln;
	opserr << "WARNING invalid tag: nDMaterial planeStressLayeredMaterial: " << 
	  tag << endln;	    	    
	return 0;
    }
	
    if (nLayers < 3) {
	opserr << "ERROR number of layers must be larger than 2" << endln;
	opserr << "nDMaterial planeStressLayeredMaterial tag: " << tag << endln;
	return 0;
    }
      
    theMats   = new NDMaterial*[nLayers];
    thickness = new double[nLayers];
      
    for (int iLayer = 0; iLayer < nLayers; iLayer++) {
	if (OPS_GetNumRemainingInputArgs() < 2) {
	  opserr << "nDMaterial planeStressLayeredMaterial tag: " << tag;
	    opserr << " WARNING must provide " << 2*nLayers <<" inputs\n";
	    return 0;
	}
	if (OPS_GetIntInput(&numdata, &matTag) < 0) {
	  opserr << "nDMaterial planeStressLayeredMaterial tag: " << tag;
	  opserr << " WARNING invalid matTag" << endln;
	  return 0;
	}
	
	theMats[iLayer] = OPS_getNDMaterial(matTag);
	if (theMats[iLayer] == 0) {
	    opserr << "nDMaterial planeStressLayeredMaterial tag: " << tag;
	    opserr << " WARNING nD material does not exist with tag: " << matTag << endln;
	    return 0;
	}

	if (OPS_GetDoubleInput(&numdata, &h) < 0) {
	  opserr << "nDMaterial planeStressLayeredMaterial tag: " << tag 
		 << " invalid h\n";
	    return 0;
	}
	
	if (h < 0) {
	  opserr << "nDMaterial planeStressLayeredMaterial tag: " << tag 
		 << " invalid h\n";
	    return 0;
	}
	
	thickness[iLayer] = h;
    }
      
    NDMaterial* theSection = new PlaneStressLayeredMaterial(tag, nLayers, thickness, theMats);

    if (thickness != 0) delete thickness;
    if (theMats != 0) delete [] theMats;

    return theSection;
}

//static vector and matrices
Vector  PlaneStressLayeredMaterial::stress(3) ;
Matrix  PlaneStressLayeredMaterial::tangent(3,3) ;

//null constructor
PlaneStressLayeredMaterial::PlaneStressLayeredMaterial() 
:NDMaterial(0, ND_TAG_PlaneStressLayeredMaterial), strain(3)
{

}

//full constructor
PlaneStressLayeredMaterial::PlaneStressLayeredMaterial(int tag,     
						 int iLayers, 
						 double *thickness, 
						 NDMaterial **fibers )
:NDMaterial(tag, ND_TAG_PlaneStressLayeredMaterial),strain(3)
{
  nLayers = iLayers;
  wg = new double[iLayers];
  theFibers = new NDMaterial*[iLayers];

  h = 0.0;

  for (int i = 0; i < iLayers; i++ ) {
    h = h + thickness[i];
    wg[i] = thickness[i];
    theFibers[i] = fibers[i]->getCopy( "PlaneStress2D" ) ;
    if (theFibers[i]==0) {
      opserr << "PlaneStressLayeredMaterial::ERROR: Could Not return a PlateFiber Material: ";
      opserr << fibers[i]->getTag() << endln;
      exit(-1);
    }
  }
}

PlaneStressLayeredMaterial::~PlaneStressLayeredMaterial( ) 
{ 
  int i ;
  if (wg != 0) delete wg;
  if (theFibers != 0) {
    for ( i = 0; i < nLayers; i++ ) {
      if (theFibers[i] != 0) delete theFibers[i] ;
    }
    delete [] theFibers;
  }
} 

NDMaterial *
PlaneStressLayeredMaterial::getCopy(const char *type) {
  if (strcmp(type,"PlaneStress") == 0 ||
      strcmp(type,"PlaneStress2D") == 0) {
    return this->getCopy();
  } else {
    opserr << "PlaneStressLayeredMaterial::getCopy() - type: " << type << " not known\n";
    return 0;
  }
}

NDMaterial  *PlaneStressLayeredMaterial::getCopy( ) 
{
  PlaneStressLayeredMaterial *clone = 0;   //new instance of this class
  clone = new PlaneStressLayeredMaterial(this->getTag(),
					 nLayers,
					 wg,
					 theFibers ) ; 
  return clone ;
}

int PlaneStressLayeredMaterial::getOrder( ) const
{
  return 3;
}


int PlaneStressLayeredMaterial::commitState( ) 
{
  int success = 0 ;
  
  for (int i = 0; i < nLayers; i++ )
    success += theFibers[i]->commitState( ) ;
  
  return success ;
}


int PlaneStressLayeredMaterial::revertToLastCommit( )
{
  int success = 0 ;
  for (int i = 0; i < nLayers; i++ )
    success += theFibers[i]->revertToLastCommit( ) ;

  strain = theFibers[0]->getStrain();
  return success ;
}

int PlaneStressLayeredMaterial::revertToStart( )
{
  int success = 0 ;

  strain.Zero();
  for (int i = 0; i < nLayers; i++ )
    success += theFibers[i]->revertToStart( ) ;

  return success ;
}

//mass per unit area
double
PlaneStressLayeredMaterial::getRho( )
{
  double rhoH = 0.0 ;
  
  for ( int i = 0; i < nLayers; i++ ) {
    rhoH += ( theFibers[i]->getRho() ) * wg[i] ;
  }

  return rhoH ;

}

//receive the strainResultant 
int PlaneStressLayeredMaterial ::
setTrialStrain( const Vector &inStrain)
{
  strain = inStrain;
  int success = 0;
  for (int i = 0; i < nLayers; i++ ) {
    success += theFibers[i]->setTrialStrain( strain ) ;
  } 
  
  return success ;
}

int 
PlaneStressLayeredMaterial ::setTrialStrain (const Vector &v, const Vector &r) {
  return this->setTrialStrain(v);
}

int 
PlaneStressLayeredMaterial ::setTrialStrainIncr (const Vector &v) {
  strain += v;
  return this->setTrialStrain(strain);
}

int 
PlaneStressLayeredMaterial ::setTrialStrainIncr (const Vector &v, const Vector &r) {
  return this->setTrialStrainIncr(v); 
}

//send back the strainResultant
const Vector& PlaneStressLayeredMaterial::getStrain( )
{
  return strain;
}




//send back the stressResultant 
const Vector&  PlaneStressLayeredMaterial::getStress( )
{
  stress.Zero();

  for (int i = 0; i < nLayers; i++ ) {
      stress += theFibers[i]->getStress( ) * wg[i];
  } 

   return stress;
}



const Matrix&  
PlaneStressLayeredMaterial::getInitialTangent( ){
  tangent.Zero();
  for (int i = 0; i < nLayers; i++ ) {
    const Matrix &dd = theFibers[i]->getInitialTangent( ) ;
    tangent.addMatrix(1.0, dd, wg[i]);
  } //end for i

  return tangent;
}


const Matrix&  
PlaneStressLayeredMaterial::getTangent( ){
  tangent.Zero();
  for (int i = 0; i < nLayers; i++ ) {
    const Matrix &dd = theFibers[i]->getTangent( ) ;
    tangent.addMatrix(1.0, dd, wg[i]);
  } //end for i

  return tangent;
}


void  PlaneStressLayeredMaterial::Print( OPS_Stream &s, int flag )
{
  s << "PlaneStressFiber Section tag: " << this->getTag() << endln ; 
  s << "Total thickness h = " << h << endln ;

  for (int i = 0; i < nLayers; i++) {
    s << "Layer " << i+1 << ", thickness h = " << 0.5 * wg[i] * h << endln;
    theFibers[i]->Print( s, flag ) ;
    s << endln;
  }
}

int 
PlaneStressLayeredMaterial::sendSelf(int commitTag, Channel &theChannel) 
{
  opserr << "PlaneStressLayeredMaterial::sendSelf() - not implemented\n";
  return -1;
}


int 
PlaneStressLayeredMaterial::recvSelf(int commitTag, Channel &theChannel, FEM_ObjectBroker &theBroker)
{
  opserr << "PlaneStressLayeredMaterial::recvSelf() - not implemented\n";
  return -1;
}
 


