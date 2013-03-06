#include <petscdmshell.h>       /*I    "petscdmshell.h"  I*/
#include <petscmat.h>
#include <petsc-private/dmimpl.h>

typedef struct  {
  Vec Xglobal;
  Vec Xlocal;
  Mat A;
  VecScatter *gtol;
  VecScatter *ltog;
} DM_Shell;

#undef __FUNCT__
#define __FUNCT__ "DMCreateMatrix_Shell"
static PetscErrorCode DMCreateMatrix_Shell(DM dm,MatType mtype,Mat *J)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;
  Mat            A;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(J,3);
  if (!shell->A) {
    if (shell->Xglobal) {
      PetscInt m,M;
      ierr = PetscInfo(dm,"Naively creating matrix using global vector distribution without preallocation");CHKERRQ(ierr);
      ierr = VecGetSize(shell->Xglobal,&M);CHKERRQ(ierr);
      ierr = VecGetLocalSize(shell->Xglobal,&m);CHKERRQ(ierr);
      ierr = MatCreate(PetscObjectComm((PetscObject)dm),&shell->A);CHKERRQ(ierr);
      ierr = MatSetSizes(shell->A,m,m,M,M);CHKERRQ(ierr);
      if (mtype) {ierr = MatSetType(shell->A,mtype);CHKERRQ(ierr);}
      ierr = MatSetUp(shell->A);CHKERRQ(ierr);
    } else SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_USER,"Must call DMShellSetMatrix(), DMShellSetCreateMatrix(), or provide a vector");
  }
  A = shell->A;
  /* the check below is tacky and incomplete */
  if (mtype) {
    PetscBool flg,aij,seqaij,mpiaij;
    ierr = PetscObjectTypeCompare((PetscObject)A,mtype,&flg);CHKERRQ(ierr);
    ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQAIJ,&seqaij);CHKERRQ(ierr);
    ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&mpiaij);CHKERRQ(ierr);
    ierr = PetscStrcmp(mtype,MATAIJ,&aij);CHKERRQ(ierr);
    if (!flg) {
      if (!(aij & (seqaij || mpiaij))) SETERRQ2(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_NOTSAMETYPE,"Requested matrix of type %s, but only %s available",mtype,((PetscObject)A)->type_name);
    }
  }
  if (((PetscObject)A)->refct < 2) { /* We have an exclusive reference so we can give it out */
    ierr = PetscObjectReference((PetscObject)A);CHKERRQ(ierr);
    ierr = MatZeroEntries(A);CHKERRQ(ierr);
    *J   = A;
  } else {                      /* Need to create a copy, could use MAT_SHARE_NONZERO_PATTERN in most cases */
    ierr = MatDuplicate(A,MAT_DO_NOT_COPY_VALUES,J);CHKERRQ(ierr);
    ierr = MatZeroEntries(*J);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateGlobalVector_Shell"
PetscErrorCode DMCreateGlobalVector_Shell(DM dm,Vec *gvec)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;
  Vec            X;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(gvec,2);
  *gvec = 0;
  X     = shell->Xglobal;
  if (!X) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_USER,"Must call DMShellSetGlobalVector() or DMShellSetCreateGlobalVector()");
  if (((PetscObject)X)->refct < 2) { /* We have an exclusive reference so we can give it out */
    ierr  = PetscObjectReference((PetscObject)X);CHKERRQ(ierr);
    ierr  = VecZeroEntries(X);CHKERRQ(ierr);
    *gvec = X;
  } else {                      /* Need to create a copy, could use MAT_SHARE_NONZERO_PATTERN in most cases */
    ierr = VecDuplicate(X,gvec);CHKERRQ(ierr);
    ierr = VecZeroEntries(*gvec);CHKERRQ(ierr);
  }
  ierr = VecSetDM(*gvec,dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateLocalVector_Shell"
PetscErrorCode DMCreateLocalVector_Shell(DM dm,Vec *gvec)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;
  Vec            X;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(gvec,2);
  *gvec = 0;
  X     = shell->Xlocal;
  if (!X) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_USER,"Must call DMShellSetLocalVector() or DMShellSetCreateLocalVector()");
  if (((PetscObject)X)->refct < 2) { /* We have an exclusive reference so we can give it out */
    ierr  = PetscObjectReference((PetscObject)X);CHKERRQ(ierr);
    ierr  = VecZeroEntries(X);CHKERRQ(ierr);
    *gvec = X;
  } else {                      /* Need to create a copy, could use MAT_SHARE_NONZERO_PATTERN in most cases */
    ierr = VecDuplicate(X,gvec);CHKERRQ(ierr);
    ierr = VecZeroEntries(*gvec);CHKERRQ(ierr);
  }
  ierr = VecSetDM(*gvec,dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetMatrix"
/*@
   DMShellSetMatrix - sets a template matrix associated with the DMShell

   Collective

   Input Arguments:
+  dm - shell DM
-  J - template matrix

   Level: advanced

.seealso: DMCreateMatrix(), DMShellSetCreateMatrix()
@*/
PetscErrorCode DMShellSetMatrix(DM dm,Mat J)
{
  DM_Shell       *shell = (DM_Shell*)dm->data;
  PetscErrorCode ierr;
  PetscBool      isshell;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(J,MAT_CLASSID,2);
  ierr = PetscObjectTypeCompare((PetscObject)dm,DMSHELL,&isshell);CHKERRQ(ierr);
  if (!isshell) PetscFunctionReturn(0);
  ierr     = PetscObjectReference((PetscObject)J);CHKERRQ(ierr);
  ierr     = MatDestroy(&shell->A);CHKERRQ(ierr);
  shell->A = J;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetCreateMatrix"
/*@C
   DMShellSetCreateMatrix - sets the routine to create a matrix associated with the shell DM

   Logically Collective on DM

   Input Arguments:
+  dm - the shell DM
-  func - the function to create a matrix

   Level: advanced

.seealso: DMCreateMatrix(), DMShellSetMatrix()
@*/
PetscErrorCode DMShellSetCreateMatrix(DM dm,PetscErrorCode (*func)(DM,MatType,Mat*))
{

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dm->ops->creatematrix = func;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetGlobalVector"
/*@
   DMShellSetGlobalVector - sets a template global vector associated with the DMShell

   Logically Collective on DM

   Input Arguments:
+  dm - shell DM
-  X - template vector

   Level: advanced

.seealso: DMCreateGlobalVector(), DMShellSetMatrix(), DMShellSetCreateGlobalVector()
@*/
PetscErrorCode DMShellSetGlobalVector(DM dm,Vec X)
{
  DM_Shell       *shell = (DM_Shell*)dm->data;
  PetscErrorCode ierr;
  PetscBool      isshell;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(X,VEC_CLASSID,2);
  ierr = PetscObjectTypeCompare((PetscObject)dm,DMSHELL,&isshell);CHKERRQ(ierr);
  if (!isshell) PetscFunctionReturn(0);
  ierr           = PetscObjectReference((PetscObject)X);CHKERRQ(ierr);
  ierr           = VecDestroy(&shell->Xglobal);CHKERRQ(ierr);
  shell->Xglobal = X;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetCreateGlobalVector"
/*@C
   DMShellSetCreateGlobalVector - sets the routine to create a global vector associated with the shell DM

   Logically Collective

   Input Arguments:
+  dm - the shell DM
-  func - the creation routine

   Level: advanced

.seealso: DMShellSetGlobalVector(), DMShellSetCreateMatrix()
@*/
PetscErrorCode DMShellSetCreateGlobalVector(DM dm,PetscErrorCode (*func)(DM,Vec*))
{

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dm->ops->createglobalvector = func;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetLocalVector"
/*@
   DMShellSetLocalVector - sets a template local vector associated with the DMShell

   Logically Collective on DM

   Input Arguments:
+  dm - shell DM
-  X - template vector

   Level: advanced

.seealso: DMCreateLocalVector(), DMShellSetMatrix(), DMShellSetCreateLocalVector()
@*/
PetscErrorCode DMShellSetLocalVector(DM dm,Vec X)
{
  DM_Shell       *shell = (DM_Shell*)dm->data;
  PetscErrorCode ierr;
  PetscBool      isshell;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(X,VEC_CLASSID,2);
  ierr = PetscObjectTypeCompare((PetscObject)dm,DMSHELL,&isshell);CHKERRQ(ierr);
  if (!isshell) PetscFunctionReturn(0);
  ierr = PetscObjectReference((PetscObject)X);CHKERRQ(ierr);
  ierr = VecDestroy(&shell->Xlocal);CHKERRQ(ierr);
  shell->Xlocal = X;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetCreateLocalVector"
/*@C
   DMShellSetCreateLocalVector - sets the routine to create a local vector associated with the shell DM

   Logically Collective

   Input Arguments:
+  dm - the shell DM
-  func - the creation routine

   Level: advanced

.seealso: DMShellSetLocalVector(), DMShellSetCreateMatrix()
@*/
PetscErrorCode DMShellSetCreateLocalVector(DM dm,PetscErrorCode (*func)(DM,Vec*))
{

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dm->ops->createlocalvector = func;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetGlobalToLocal"
/*@C
   DMShellSetGlobalToLocal - Sets the routines used to perform a global to local scatter

   Logically Collective on DM

   Input Arguments
+  dm - the shell DM
.  begin - the routine that begins the global to local scatter
-  end - the routine that ends the global to local scatter

   Level: advanced

.seealso: DMShellSetLocalToGlobal()
@*/
PetscErrorCode DMShellSetGlobalToLocal(DM dm,PetscErrorCode (*begin)(DM,Vec,InsertMode,Vec),PetscErrorCode (*end)(DM,Vec,InsertMode,Vec)) {
  PetscFunctionBegin;
  dm->ops->globaltolocalbegin = begin;
  dm->ops->globaltolocalend = end;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetLocalToGlobal"
/*@C
   DMShellSetLocalToGlobal - Sets the routines used to perform a local to global scatter

   Logically Collective on DM

   Input Arguments
+  dm - the shell DM
.  begin - the routine that begins the local to global scatter
-  end - the routine that ends the local to global scatter

   Level: advanced

.seealso: DMShellSetGlobalToLocal()
@*/
PetscErrorCode DMShellSetLocalToGlobal(DM dm,PetscErrorCode (*begin)(DM,Vec,InsertMode,Vec),PetscErrorCode (*end)(DM,Vec,InsertMode,Vec)) {
  PetscFunctionBegin;
  dm->ops->localtoglobalbegin = begin;
  dm->ops->localtoglobalend = end;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellSetGlobalToLocalVecScatter"
/*@
   DMShellSetGlobalToLocalVecScatter - Sets a VecScatter context for global to local communication

   Logically Collective on DM

   Input Arguments
+  dm - the shell DM
-  gtol - the global to local VecScatter context

   Level: advanced

.seealso: DMShellSetGlobalToLocal()
@*/
PetscErrorCode DMShellSetGlobalToLocalVecScatter(DM dm, VecScatter *gtol)
{
  DM_Shell       *shell = (DM_Shell*)dm->data;

  PetscFunctionBegin;
  shell->gtol = gtol;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDestroy_Shell"
static PetscErrorCode DMDestroy_Shell(DM dm)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;

  PetscFunctionBegin;
  ierr = MatDestroy(&shell->A);CHKERRQ(ierr);
  ierr = VecDestroy(&shell->Xglobal);CHKERRQ(ierr);
  ierr = VecDestroy(&shell->Xlocal);CHKERRQ(ierr);
  /* This was originally freed in DMDestroy(), but that prevents reference counting of backend objects */
  ierr = PetscFree(shell);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMView_Shell"
static PetscErrorCode DMView_Shell(DM dm,PetscViewer v)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;

  PetscFunctionBegin;
  ierr = VecView(shell->Xglobal,v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLoad_Shell"
static PetscErrorCode DMLoad_Shell(DM dm,PetscViewer v)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;

  PetscFunctionBegin;
  ierr = VecCreate(PetscObjectComm((PetscObject)dm),&shell->Xglobal);CHKERRQ(ierr);
  ierr = VecLoad(shell->Xglobal,v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreate_Shell"
PETSC_EXTERN PetscErrorCode DMCreate_Shell(DM dm)
{
  PetscErrorCode ierr;
  DM_Shell       *shell;

  PetscFunctionBegin;
  ierr     = PetscNewLog(dm,DM_Shell,&shell);CHKERRQ(ierr);
  dm->data = shell;

  ierr = PetscObjectChangeTypeName((PetscObject)dm,DMSHELL);CHKERRQ(ierr);

  dm->ops->destroy            = DMDestroy_Shell;
  dm->ops->createglobalvector = DMCreateGlobalVector_Shell;
  dm->ops->createlocalvector  = DMCreateLocalVector_Shell;
  dm->ops->creatematrix       = DMCreateMatrix_Shell;
  dm->ops->view               = DMView_Shell;
  dm->ops->load               = DMLoad_Shell;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellCreate"
/*@
    DMShellCreate - Creates a shell DM object, used to manage user-defined problem data

    Collective on MPI_Comm

    Input Parameter:
.   comm - the processors that will share the global vector

    Output Parameters:
.   shell - the shell DM

    Level: advanced

.seealso DMDestroy(), DMCreateGlobalVector(), DMCreateLocalVector()
@*/
PetscErrorCode  DMShellCreate(MPI_Comm comm,DM *dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(dm,2);
  ierr = DMCreate(comm,dm);CHKERRQ(ierr);
  ierr = DMSetType(*dm,DMSHELL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellDefaultGlobalToLocalBegin"
/*@
   DMShellDefaultGlobalToLocalBegin - Uses the GlobalToLocal VecScatter context set by the user to begin a global to local scatter
   Collective

   Input Arguments:
+  dm - shell DM
.  g - global vector
.  mode - InsertMode
-  l - local vector

   Level: advanced

.seealso: DMShellDefaultGlobalToLocalEnd()
@*/
PetscErrorCode DMShellDefaultGlobalToLocalBegin(DM dm,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;

  PetscFunctionBegin;
  ierr = VecScatterBegin(*(shell->gtol),g,l,mode,SCATTER_FORWARD);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMShellDefaultGlobalToLocalEnd"
/*@
   DMShellDefaultGlobalToLocalEnd - Uses the GlobalToLocal VecScatter context set by the user to end a global to local scatter
   Collective

   Input Arguments:
+  dm - shell DM
.  g - global vector
.  mode - InsertMode
-  l - local vector

   Level: advanced

.seealso: DMShellDefaultGlobalToLocalBegin()
@*/
PetscErrorCode DMShellDefaultGlobalToLocalEnd(DM dm,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode ierr;
  DM_Shell       *shell = (DM_Shell*)dm->data;

  PetscFunctionBegin;
  ierr = VecScatterEnd(*(shell->gtol),g,l,mode,SCATTER_FORWARD);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
