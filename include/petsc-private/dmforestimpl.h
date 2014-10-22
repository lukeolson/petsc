#if !defined(_FORESTIMPL_H)
#define _FORESTIMPL_H

#include <petscmat.h>       /*I      "petscmat.h"          I*/
#include <petscdmforest.h> /*I      "petscdmforest.h"    I*/
#include <petscbt.h>
#include <petsc-private/dmimpl.h>
#include <../src/sys/utils/hash.h>

typedef struct {
  PetscInt                   refct;
  void                       *data;
  PetscErrorCode             (*createcellchart)(DM,PetscInt*,PetscInt*);
  PetscErrorCode             (*createcellsf)(DM,PetscSF*);
  PetscBool                  setFromOptions;
  PetscBool                  setup;
  DMForestTopology           topology;
  DM                         base;
  DM                         coarse;
  DM                         fine;
  PetscInt                   adjDim;
  PetscInt                   overlap;
  PetscInt                   minRefinement;
  PetscInt                   maxRefinement;
  PetscInt                   cStart;
  PetscInt                   cEnd;
  PetscSF                    cellSF;
  PetscInt                   *adaptMarkers;
  PetscCopyMode              adaptCopyMode;
  DMForestAdaptivityStrategy adaptStrategy;
  PetscInt                   gradeFactor;
  PetscReal                  *cellWeights;
  PetscCopyMode              cellWeightsCopyMode;
  PetscReal                  weightsFactor;
  PetscReal                  weightCapacity;
} DM_Forest;

#endif /* _FORESTIMPL_H */
