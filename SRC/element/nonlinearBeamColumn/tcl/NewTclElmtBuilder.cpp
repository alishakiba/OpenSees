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
// $Date: 2003-02-14 23:01:17 $
// $Source: /usr/local/cvs/OpenSees/SRC/element/nonlinearBeamColumn/tcl/NewTclElmtBuilder.cpp,v $
                                                                        
                                                                        
// File: ~/tcl/TclElmtBuilder.C
// 
// Written: Remo M. de Souza (rmsouza@ce.berkeley.edu)
// Created: 08/99
// based on TclPlaneFrame.C by fmk and rms
//
// Description: This file contains the implementation of the commands used 
// to add sections and nonlinear frame elements to the model.

#include <stdlib.h>
#include <string.h>
#include <iOPS_Stream.h>

#include <Domain.h>
#include <Node.h>
#include <ArrayOfTaggedObjects.h>
#include <Matrix.h>

#include <SectionForceDeformation.h>

#include <NLBeamColumn2d.h>
#include <NLBeamColumn3d.h>

#include <LinearCrdTransf2d.h>
#include <CorotCrdTransf2d.h>
#include <LinearCrdTransf3d.h>
#include <CorotCrdTransf3d.h>

#include <TclModelBuilder.h>


#define ARRAYSIZE 30

//
// some static variables used in the functions
//

static Domain *theTclModelBuilderDomain = 0;
static TclModelBuilder *theTclModelBuilder =0;

// 
// to create a NL frame element and add to the domain
//
int
TclModelBuilder_addFrameElement(ClientData clientData, Tcl_Interp *interp,
				int inArgc, char **inArgv,
				Domain *theDomain,
				TclModelBuilder *theBuilder)
				
{
  theTclModelBuilderDomain = theDomain;
  theTclModelBuilder = theBuilder;
    
  int NDM, NDF;
     
  NDM = theTclModelBuilder->getNDM();   // dimension of the structure (1d, 2d, or 3d)
  NDF = theTclModelBuilder->getNDF();   // number of degrees of freedom per node

  // split possible lists present in argv
  char *List;

  List = Tcl_Merge (inArgc, inArgv);
  if (List == 0)
  {
      blah->result = "WARNING - TclModelBuilder_addFrameElement - problem merging list";
      return TCL_ERROR;
  }

//  opserr << "List :" << List << endln;

  // remove braces from list
  for (int i = 0; List[i] != '\0'; i++)
  {
    if ((List[i] == '{')  ||  (List[i] == '}'))
      List[i] = ' ';
  }
  
  int argc;
  char **argv;
       
  if (Tcl_SplitList(interp, List, &argc, &argv) != TCL_OK)
  {
     blah->result = "WARNING - TclModelBuilder_addFrameElement - problem spliting list";
     return TCL_ERROR;
  }
      
  Tcl_Free (List);
  
//  opserr << "argc : " << argc; 
//  for (int i=0; i<argc; i++)
//  {
//    opserr <<"string " << i << " : " << argv[i] << endln;
//  }


  // create plane frame elements
  if (NDM == 2 && NDF == 3)     
  {
       
    int eleTag, iNode, jNode, integrTag, transfTag;

    if (argc < 7)
    {
      blah->result = "WARNING bad command - want: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }
    int argi = 2;  
    if (Tcl_GetInt(interp, argv[argi++], &eleTag) != TCL_OK)
    {
      blah->result = "WARNING invalid eleTag: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>"; 
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &iNode) != TCL_OK)
    {
      blah->result = "WARNING invalid iNode:  element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &jNode) != TCL_OK)
    {
      blah->result = "WARNING invalid jNode: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &intgrTag) != TCL_OK)
    {
      blah->result = "WARNING invalid numIntgrPts: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &transfTag) != TCL_OK)
    {
      blah->result = "WARNING invalid transfTag? - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }
    
    // allow some additional options at end of command
    double massDens = 0;
    int    nMaxLocIters = 1;
    double locToler = 1e-16;
 
    while (argi != argc) 
    {
      if (strcmp(argv[argi],"-mass") == 0) 
      {
         // allow user to specify mass (per unit length)
         argi++;
         if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &massDens) != TCL_OK)
         {
            blah->result = "WARNING invalid massDens - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
            return TCL_ERROR;
         } 
      }

      else if (strcmp(argv[argi],"-iter") == 0) 
      {
         // allow user to specify maximum number of local iterations
         argi++;
         if (argi == argc || Tcl_GetInt(interp, argv[argi++], &nMaxLocIters) != TCL_OK)
         {
	    blah->result = "WARNING invalid nMaxLocIters - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
            return TCL_ERROR;
         } 

         // specify local tolerance 
         if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &locToler) != TCL_OK)
         {
            blah->result = "WARNING invalid locToler - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
            return TCL_ERROR;
         } 
      }
      else
      {
         blah->result = "WARNING bad command  - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
         opserr << "invalid: " << argv[argi] << endln;
         return TCL_ERROR;
      }
    }

    
    CrdTransf2d *theCrdTransf = theTclModelBuilder->getCrdTransf2d(transfTag);

    if (theCrdTransf == 0) 
    {
       opserr << "WARNING TclElmtBuilder - frameElement - no geometric transformation found with tag ";
       opserr << transfTag << endln;
       return TCL_ERROR;
    }
        
    
    BeamColumnIntegration *theIntegration = theTclModelBuilder->getBeamColumnIntegr(integrTag);

    if (theIntegration == 0) 
    {
       opserr << "WARNING TclElmtBuilder - frameElement - no element integration found with tag ";
       opserr << integrTag << endln;
       return TCL_ERROR;
    }
/*      
    SectionForceDeformation *theSection = theTclModelBuilder->getSection(secTag);

    if (theSection == 0) 
    {
       opserr << "WARNING TclElmtBuilder - frameElement - no Section found with tag ";
       opserr << secTag << endln;
       return TCL_ERROR;
    }

    // get pointer to the sections for the whole beam

    SectionForceDeformation **sections = new SectionForceDeformation* [numIntgrPts];
    
    if (!sections)
    {
      blah->result = "WARNING TclElmtBuilder - addFrameElement - Insufficient memory to create sections";
      return TCL_ERROR;
    }

    for (int j=0; j<numIntgrPts; j++)
       sections[j] = theSection;
*/
    
    // opserr << "massDens " << massDens << endln;
     
    // construct the element
    
    Element *element;
    if (strcmp(argv[1],"nonlinearBeamColumn") == 0)
    {
       element = new NLBeamColumn2d (eleTag, iNode, jNode, *theIntegration, *theCrdTransf, massDens, nMaxLocIters, locToler);

       delete [] sections;
    }   
    else
    {
      blah->result = "WARNING TclElmtBuilder - addFrameElement - invalid elemType";
      return TCL_ERROR;
    }
     
    if (element == 0)
    {
      blah->result = "WARNING  TclElmtBuilder - addFrameElement - ran out of memory to create element";
      return TCL_ERROR;
    }
   
   if (theTclModelBuilderDomain->addElement(element) == false) 
   {
      opserr << "WARNING TclElmtBuilder - addFrameElement - could not add element to domain ";
      opserr << eleTag << endln;
      return TCL_ERROR;
    } 
  }
  
  // create 3d frame element 

  else if (NDM == 3 && NDF == 6)
  {
    int   eleTag, iNode, jNode, numIntgrPts, secTag, transfTag;
        
    if (argc < 8)
    {
      blah->result = "WARNING bad command - want: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }
    int argi = 2;
    if (Tcl_GetInt(interp, argv[argi++], &eleTag) != TCL_OK)
    {
      blah->result = "WARNING invalid eleTag: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &iNode) != TCL_OK)
    {
      blah->result = "WARNING invalid iNode: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &jNode) != TCL_OK)
    {
      blah->result = "WARNING invalid jNode: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &numIntgrPts) != TCL_OK)
    {
      blah->result = "WARNING invalid numIntgrPts: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[argi++], &secTag) != TCL_OK)
    {
      blah->result = "WARNING invalid secTag: element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }
   
    if (Tcl_GetInt(interp, argv[argi++], &transfTag) != TCL_OK)
    {
      blah->result = "WARNING invalid transfTag - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
      return TCL_ERROR;
    }
    
    // allow some additional options at end of command
    double massDens = 0;
    int    nMaxLocIters = 1;
    double locToler = 1e-16;
 
    while (argi != argc) 
    {
      if (strcmp(argv[argi],"-mass") == 0) 
      {
         // allow user to specify mass (per unit length)
         argi++;
         if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &massDens) != TCL_OK)
         {
            blah->result = "WARNING invalid massDens - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
            return TCL_ERROR;
         } 
      }

      else if (strcmp(argv[argi],"-iter") == 0) 
      {
         // allow user to specify maximum number of local iterations
         argi++;
         if (argi == argc || Tcl_GetInt(interp, argv[argi++], &nMaxLocIters) != TCL_OK)
         {
	    blah->result = "WARNING invalid nMaxLocIters - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
            return TCL_ERROR;
         } 

         // specify local tolerance 
         if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &locToler) != TCL_OK)
         {
            blah->result = "WARNING invalid locToler - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>";
            return TCL_ERROR;
         } 
      }
      else
      {
         blah->result = "WARNING bad command  - element nonlinearBeamColumn eleTag? iNode? jNode? integrTag? transfTag? <-mass massDens?> <-iter nMaxLocIters? locToler?>"; 
         opserr << "invalid: " << argv[argi] << endln;
         return TCL_ERROR;
      }
    }
    
   
    CrdTransf3d *theCrdTransf = theTclModelBuilder->getCrdTransf3d(transfTag);

    if (theCrdTransf == 0) 
    {
       opserr << "WARNING TclElmtBuilder - frameElement - no geometric transformation found with tag ";
       opserr << transfTag << endln;
       return TCL_ERROR;
    }
    
    SectionForceDeformation *theSection = theTclModelBuilder->getSection(secTag);

    if (theSection == 0) 
    {
       opserr << "WARNING Tcl3dFrame - frameElement - no Section found with tag";
       opserr << secTag << endln;
       return TCL_ERROR;
    }

    // create the element

    // get pointer to the sections for the whole beam

    SectionForceDeformation **sections = new SectionForceDeformation* [numIntgrPts];
    
    if (!sections)
    {
      blah->result = "WARNING TclElmtBuilder - addFrameElement - Insufficient memory to create sections";
      return TCL_ERROR; 
    }

    for (int j=0; j<numIntgrPts; j++)
       sections[j] = theSection;
    
    // construct the element
    
    Element *element;

    if (strcmp(argv[1],"nonlinearBeamColumn") == 0)
    {
	element = new NLBeamColumn3d (eleTag, iNode, jNode, numIntgrPts, sections, *theCrdTransf, massDens, nMaxLocIters, locToler);
	 
	delete [] sections;
    }
    else
    {
       blah->result = "WARNING TclElmtBuilder - addFrameElement - invalid elemType";
       return TCL_ERROR;
    }
     
    if (element == 0)
    {
      blah->result = "WARNING  TclElmtBuilder - addFrameElement - ran out of memory to create element";
      return TCL_ERROR;
    }
   
    if (theTclModelBuilderDomain->addElement(element) == false) 
    {
      opserr << "WARNING TclElmtBuilder - addFrameElement - could not add element to domain ";
      opserr << eleTag << endln;
      return TCL_ERROR;
    }
    
  }
  
  else
  {
     opserr << "WARNING NDM = " << NDM << " and NDF = " << NDF << "is imcompatible with available frame elements";
     return TCL_ERROR;
  }      

  Tcl_Free ((char *)argv);
        
  // if get here we have sucessfully created the element and added it to the domain

  return TCL_OK;
}






// 
// to create a coordinate transformation 
//
int
TclModelBuilder_addGeomTransf(ClientData clientData, Tcl_Interp *interp,
				int argc, char **argv,
				Domain *theDomain,
				TclModelBuilder *theBuilder)
				
{
   theTclModelBuilderDomain = theDomain;
   theTclModelBuilder = theBuilder;
    
   int NDM, NDF;
     
   NDM = theTclModelBuilder->getNDM();   // dimension of the structure (1d, 2d, or 3d)
   NDF = theTclModelBuilder->getNDF();   // number of degrees of freedom per node

   // create 2d coordinate transformation
   if (NDM == 2 && NDF == 3)     
   {
      if ((strcmp(argv[1],"Linear") == 0) || (strcmp(argv[1],"LinearWithPDelta") == 0) || (strcmp(argv[1],"Corotational") == 0))
      {
	 int crdTransfTag;
         Vector jntOffsetI(2), jntOffsetJ(2);
	 
	 if (argc < 3) 
	 {
       	    blah->result = "WARNING insufficient arguments - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>"; 
	    return TCL_ERROR;
	 }
	    
         int argi = 2;  
         if (Tcl_GetInt(interp, argv[argi++], &crdTransfTag) != TCL_OK)
	 {	
	    blah->result = "WARNING invalid tag - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>";
	    return  TCL_ERROR;
	 }

	 // allow additional options at end of command
	 int i;

	 while (argi != argc) 
         {
	    if (strcmp(argv[argi],"-jntOffset") == 0) 
            {
               for (i = 0; i < 2; i++)
	       {
                  if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetI(i)) != TCL_OK) 
                  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>";
		     return TCL_ERROR;
                  }
	       }
 
	       for (i = 0; i < 2; i++)
               {
	          if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetJ(i)) != TCL_OK) 
		  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>";
		     return TCL_ERROR;
		  }
               }
	    }
	 
	    else
	    {
               blah->result = "WARNING bad command - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>";
               opserr << "invalid: " << argv[argi] << endln;
               return TCL_ERROR;
            }
         }

	 // construct the transformation object
    
         CrdTransf2d *crdTransf2d;

	 if (strcmp(argv[1],"Linear") == 0)
     	    crdTransf2d = new LinearCrdTransf2d(crdTransfTag, jntOffsetI, jntOffsetJ, 0);

	 else if (strcmp(argv[1],"LinearWithPDelta") == 0)
     	    crdTransf2d = new LinearCrdTransf2d(crdTransfTag, jntOffsetI, jntOffsetJ, 1);

	 else if (strcmp(argv[1],"Corotational") == 0)
     	    crdTransf2d = new CorotCrdTransf2d(crdTransfTag, jntOffsetI, jntOffsetJ);
	 else
         {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid Type";
	    return TCL_ERROR;
	 }
     
	 if (crdTransf2d == 0)
	 {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - ran out of memory to create geometric transformation object";
	    return TCL_ERROR;
	 }

	 // add the transformation to the modelBuilder
	 if (theTclModelBuilder->addCrdTransf2d(*crdTransf2d)) 
	 {
             blah->result = "WARNING TclElmtBuilder - addGeomTransf  - could not add geometric transformation to model Builder";
             return TCL_ERROR;
         }
      }
      else
      {
         blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid geomTransf type";
         return TCL_ERROR;
      }
   }
   else if  (NDM == 3 && NDF == 6)
   {
      if ((strcmp(argv[1],"Linear") == 0) || (strcmp(argv[1],"LinearWithPDelta") == 0) || (strcmp(argv[1],"Corotational") == 0))
      { 
	 int crdTransfTag;
         Vector vecxzPlane(3);                  // vector that defines local xz plane
         Vector jntOffsetI(3), jntOffsetJ(3);   // joint offsets in global coordinates

	 if (argc < 6) 
	 {
       	    blah->result = "WARNING insufficient arguments - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	    return TCL_ERROR;
	 }
	    
         int argi = 2;  
         if (Tcl_GetInt(interp, argv[argi++], &crdTransfTag) != TCL_OK)
	 {	
	    blah->result = "WARNING invalid tag - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	    return  TCL_ERROR;
	 }

	 if (Tcl_GetDouble(interp, argv[argi++], &vecxzPlane(0)) != TCL_OK)
	 {
             blah->result = "WARNING invalid vecxzPlaneX - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	     return TCL_ERROR;
	 }
   
         if (Tcl_GetDouble(interp, argv[argi++], &vecxzPlane(1)) != TCL_OK)
	 {
             blah->result = "WARNING invalid vecxzPlaneY - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	     return TCL_ERROR;
	 }
  
         if (Tcl_GetDouble(interp, argv[argi++], &vecxzPlane(2)) != TCL_OK)
	 {
             blah->result = "WARNING invalid vecxzPlaneZ - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	     return TCL_ERROR;
	 }
  
	 // allow additional options at end of command
	 int i;

	 while (argi != argc) 
         {
	    if (strcmp(argv[argi],"-jntOffset") == 0) 
            {
               
               for (i = 0; i < 3; i++)
	       {
                  if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetI(i)) != TCL_OK) 
                  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >"; 
		     return TCL_ERROR;
                  }
	       }
 
	       for (i = 0; i < 3; i++)
               {
	          if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetJ(i)) != TCL_OK) 
		  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";  
		     return TCL_ERROR;
		  }
               }
	    }
	 
	    else
	    {
               blah->result = "WARNING bad command - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >"; 
               opserr << "invalid: " << argv[argi] << endln;
               return TCL_ERROR;
            }
         }

	 // construct the transformation object
    
         CrdTransf3d *crdTransf3d;
	 
	 opserr << " transftype: " << argv[1];

	 if (strcmp(argv[1],"Linear") == 0)
     	    crdTransf3d = new LinearCrdTransf3d(crdTransfTag, vecxzPlane, jntOffsetI, jntOffsetJ, 0);

	 else if (strcmp(argv[1],"LinearWithPDelta") == 0)
     	    crdTransf3d = new LinearCrdTransf3d(crdTransfTag, vecxzPlane, jntOffsetI, jntOffsetJ, 1);

	 else if (strcmp(argv[1],"Corotational") == 0)
     	    crdTransf3d = new CorotCrdTransf3d(crdTransfTag, vecxzPlane, jntOffsetI, jntOffsetJ);
	 else
         {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid Type";
	    return TCL_ERROR;
	 }
     
	 if (crdTransf3d == 0)
	 {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - ran out of memory to create geometric transformation object";
	    return TCL_ERROR;
	 }
	 opserr << "crdTransf3d: " << *crdTransf3d;

	 // add the transformation to the modelBuilder
	 if (theTclModelBuilder->addCrdTransf3d(*crdTransf3d)) 
	 {
             blah->result = "WARNING TclElmtBuilder - addGeomTransf  - could not add geometric transformation to model Builder";

             return TCL_ERROR;
         }
      }
      else 
      {
         blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid geomTransf type ";
         return TCL_ERROR;
      }
  }
  else
  {
     opserr << "WARNING NDM = " << NDM << " and NDF = " << NDF << "is imcompatible with available frame elements";
     return TCL_ERROR;
  }      

  Tcl_Free ((char *)argv);
        
  // if get here we have sucessfully created the element and added it to the domain

  return TCL_OK;
}







// 
// to create a beam column integration scheme
//
int
TclModelBuilder_addBeamColumnIntegrat(ClientData clientData, Tcl_Interp *interp,
				int argc, char **argv,
				Domain *theDomain,
				TclModelBuilder *theBuilder)
				
{
   theTclModelBuilderDomain = theDomain;
   theTclModelBuilder = theBuilder;
    

      if ((strcmp(argv[1],"Gauss") == 0) || (strcmp(argv[1],"Lobatto") == 0))
      {
	 int integrTag, numIntgrPts;
	 
	 if (argc < 5) 
	 {
       	    blah->result = "WARNING insufficient arguments - want: elemIntegrat type? tag? numIntgrPts? sectTag1? sectTag2? ... sectTagN?"; 
	    return TCL_ERROR;
	 }
	    
         int argi = 2;  
         if (Tcl_GetInt(interp, argv[argi++], &integrTag) != TCL_OK)
	 {	
	    blah->result = "WARNING invalid tag - want: elemIntegrat type? tag? numIntgrPts? sectTag1? sectTag2? ... sectTagN?"; 
	    return  TCL_ERROR;
	 }

         if (Tcl_GetInt(interp, argv[argi++], &numIntgrPts) != TCL_OK)
	 {	
	    blah->result = "WARNING invalid tag - want: elemIntegrat type? tag? numIntgrPts? sectTag1? sectTag2? ... sectTagN?"; 
	    return  TCL_ERROR;
	 }

	 if (integrTag < 2)
	 {	
	    blah->result = "WARNING invalid numIntgrPts. Must be > 1";
	    return  TCL_ERROR;
	 }    
	 ID sectTags(numIntgrPts);
	 
	 if (Tcl_GetInt(interp, argv[argi++], &sectTags(0)) != TCL_OK)
	 {	
	    blah->result = "WARNING invalid tag - want: elemIntegrat type? tag? numIntgrPts? sectTag1? sectTag2? ... sectTagN?"; 
	    return  TCL_ERROR;
	 }
	 
   
	 
	 // allow additional section tags at end of command for nonprismatic beams
	 int i;

	 while (argi != argc) 
         {
	    if (strcmp(argv[argi],"-jntOffset") == 0) 
            {
               for (i = 0; i < 2; i++)
	       {
                  if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetI(i)) != TCL_OK) 
                  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>";
		     return TCL_ERROR;
                  }
	       }
 
	       for (i = 0; i < 2; i++)
               {
	          if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetJ(i)) != TCL_OK) 
		  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>";
		     return TCL_ERROR;
		  }
               }
	    }
	 
	    else
	    {
               blah->result = "WARNING bad command - want: geomTransf type? tag? <-jntOffset dXi? dYi? dXj? dYj?>";
               opserr << "invalid: " << argv[argi] << endln;
               return TCL_ERROR;
            }
         }

	 // construct the transformation object
    
         CrdTransf2d *crdTransf2d;

	 if (strcmp(argv[1],"Linear") == 0)
     	    crdTransf2d = new LinearCrdTransf2d(crdTransfTag, jntOffsetI, jntOffsetJ, 0);

	 else if (strcmp(argv[1],"LinearWithPDelta") == 0)
     	    crdTransf2d = new LinearCrdTransf2d(crdTransfTag, jntOffsetI, jntOffsetJ, 1);

	 else if (strcmp(argv[1],"Corotational") == 0)
     	    crdTransf2d = new CorotCrdTransf2d(crdTransfTag, jntOffsetI, jntOffsetJ);
	 else
         {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid Type";
	    return TCL_ERROR;
	 }
     
	 if (crdTransf2d == 0)
	 {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - ran out of memory to create geometric transformation object";
	    return TCL_ERROR;
	 }

	 // add the transformation to the modelBuilder
	 if (theTclModelBuilder->addCrdTransf2d(*crdTransf2d)) 
	 {
             blah->result = "WARNING TclElmtBuilder - addGeomTransf  - could not add geometric transformation to model Builder";
             return TCL_ERROR;
         }
      }
      else
      {
         blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid geomTransf type";
         return TCL_ERROR;
      }
   }
   else if  (NDM == 3 && NDF == 6)
   {
      if ((strcmp(argv[1],"Linear") == 0) || (strcmp(argv[1],"LinearWithPDelta") == 0) || (strcmp(argv[1],"Corotational") == 0))
      { 
	 int crdTransfTag;
         Vector vecxzPlane(3);                  // vector that defines local xz plane
         Vector jntOffsetI(3), jntOffsetJ(3);   // joint offsets in global coordinates

	 if (argc < 6) 
	 {
       	    blah->result = "WARNING insufficient arguments - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	    return TCL_ERROR;
	 }
	    
         int argi = 2;  
         if (Tcl_GetInt(interp, argv[argi++], &crdTransfTag) != TCL_OK)
	 {	
	    blah->result = "WARNING invalid tag - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	    return  TCL_ERROR;
	 }

	 if (Tcl_GetDouble(interp, argv[argi++], &vecxzPlane(0)) != TCL_OK)
	 {
             blah->result = "WARNING invalid vecxzPlaneX - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	     return TCL_ERROR;
	 }
   
         if (Tcl_GetDouble(interp, argv[argi++], &vecxzPlane(1)) != TCL_OK)
	 {
             blah->result = "WARNING invalid vecxzPlaneY - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	     return TCL_ERROR;
	 }
  
         if (Tcl_GetDouble(interp, argv[argi++], &vecxzPlane(2)) != TCL_OK)
	 {
             blah->result = "WARNING invalid vecxzPlaneZ - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";
	     return TCL_ERROR;
	 }
  
	 // allow additional options at end of command
	 int i;

	 while (argi != argc) 
         {
	    if (strcmp(argv[argi],"-jntOffset") == 0) 
            {
               
               for (i = 0; i < 3; i++)
	       {
                  if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetI(i)) != TCL_OK) 
                  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >"; 
		     return TCL_ERROR;
                  }
	       }
 
	       for (i = 0; i < 3; i++)
               {
	          if (argi == argc || Tcl_GetDouble(interp, argv[argi++], &jntOffsetJ(i)) != TCL_OK) 
		  {
                     blah->result = "WARNING invalid jntOffset value - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >";  
		     return TCL_ERROR;
		  }
               }
	    }
	 
	    else
	    {
               blah->result = "WARNING bad command - want: geomTransf type? tag? vecxzPlaneX? vecxzPlaneY? vecxzPlaneZ?  <-jntOffset dXi? dYi? dZi? dXj? dYj? dZj? >"; 
               opserr << "invalid: " << argv[argi] << endln;
               return TCL_ERROR;
            }
         }

	 // construct the transformation object
    
         CrdTransf3d *crdTransf3d;
	 
	 opserr << " transftype: " << argv[1];

	 if (strcmp(argv[1],"Linear") == 0)
     	    crdTransf3d = new LinearCrdTransf3d(crdTransfTag, vecxzPlane, jntOffsetI, jntOffsetJ, 0);

	 else if (strcmp(argv[1],"LinearWithPDelta") == 0)
     	    crdTransf3d = new LinearCrdTransf3d(crdTransfTag, vecxzPlane, jntOffsetI, jntOffsetJ, 1);

	 else if (strcmp(argv[1],"Corotational") == 0)
     	    crdTransf3d = new CorotCrdTransf3d(crdTransfTag, vecxzPlane, jntOffsetI, jntOffsetJ);
	 else
         {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid Type";
	    return TCL_ERROR;
	 }
     
	 if (crdTransf3d == 0)
	 {
            blah->result = "WARNING TclElmtBuilder - addGeomTransf - ran out of memory to create geometric transformation object";
	    return TCL_ERROR;
	 }
	 opserr << "crdTransf3d: " << *crdTransf3d;

	 // add the transformation to the modelBuilder
	 if (theTclModelBuilder->addCrdTransf3d(*crdTransf3d)) 
	 {
             blah->result = "WARNING TclElmtBuilder - addGeomTransf  - could not add geometric transformation to model Builder";

             return TCL_ERROR;
         }
      }
      else 
      {
         blah->result = "WARNING TclElmtBuilder - addGeomTransf - invalid geomTransf type ";
         return TCL_ERROR;
      }
  }
  else
  {
     opserr << "WARNING NDM = " << NDM << " and NDF = " << NDF << "is imcompatible with available frame elements";
     return TCL_ERROR;
  }      

  Tcl_Free ((char *)argv);
        
  // if get here we have sucessfully created the element and added it to the domain

  return TCL_OK;
}



