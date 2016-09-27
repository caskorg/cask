#include <petscksp.h>

int main(int argc,char **args) {
  Vec            x, b, u;
  Mat            A;
  PetscErrorCode ierr;
  PetscInt       i,n,its;
  PetscMPIInt    size;
  char           filein[PETSC_MAX_PATH_LEN], buf[PETSC_MAX_PATH_LEN], vfilein[PETSC_MAX_PATH_LEN];
  FILE           *file;
  int            *Is, *Js, *rownz;
  PetscScalar    *VALs;

  PetscInitialize(&argc,&args,(char*)0, "Solve linear system in parallel, loaded from Matrix Market format");
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  if (size != 1) SETERRQ(PETSC_COMM_WORLD,1,"This is a uniprocessor example only!");

  // --- Load system matrix
  PetscBool found;
  ierr = PetscOptionsGetString(NULL,NULL,"-fin",filein,PETSC_MAX_PATH_LEN,&found);CHKERRQ(ierr);
  ierr = PetscFOpen(PETSC_COMM_SELF,filein,"r",&file);CHKERRQ(ierr);
  // TODO should use mmread library
  do fgets(buf,PETSC_MAX_PATH_LEN-1,file);
  while (buf[0] == '%');
  int nnz, m;
  sscanf(buf,"%d %d %d\n",&m,&n,&nnz);
  ierr = PetscPrintf (PETSC_COMM_SELF,"m = %d, n = %d, nnz = %d\n",m,n,nnz);

  ierr = PetscMalloc4(nnz,&Is,nnz,&Js,nnz,&VALs,m,&rownz);CHKERRQ(ierr);
  for (i=0; i<m; i++) rownz[i] = 1; // [> add 0.0 to diagonal entries <]

  for (i=0; i<nnz; i++) {
    ierr = fscanf(file,"%d %d %le\n",&Is[i],&Js[i],(double*)&VALs[i]);
    if (ierr == EOF) {
      printf("Warning!! Reached EOF, %d\n", i);
      nnz = i;
      break;
    }
    /*if (ierr == EOF) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_USER,"i=%d, reach EOF\n",i);*/
    Is[i]--; Js[i]--;    // [> adjust from 1-based to 0-based <]
    rownz[Js[i]]++;
  }
  fclose(file);

  ierr = MatCreate(PETSC_COMM_SELF,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,m,n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSeqSBAIJSetPreallocation(A,1,0,rownz);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  /* Add zero to diagonals, in case the matrix missing diagonals */
  //for (i=0; i<m; i++){
  //  ierr = MatSetValues(A,1,&i,1,&i,&zero,INSERT_VALUES);CHKERRQ(ierr);
  //}
  for (i=0; i<nnz; i++) {
    ierr = MatSetValues(A,1,&Js[i],1,&Is[i],&VALs[i],INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  // MatView(A, PETSC_VIEWER_DRAW_SELF);
  ierr = PetscPrintf(PETSC_COMM_SELF,"Assemble SBAIJ matrix completes.\n");CHKERRQ(ierr);

  // --- Load RHS
  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) x, "Solution");CHKERRQ(ierr);
  ierr = VecSetSizes(x,PETSC_DECIDE,m);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&b);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&u);CHKERRQ(ierr);
  ierr = PetscOptionsGetString(NULL,NULL,"-vin",vfilein,PETSC_MAX_PATH_LEN,&found);CHKERRQ(ierr);
  ierr = PetscFOpen(PETSC_COMM_SELF,vfilein,"r",&file);CHKERRQ(ierr);
  do fgets(buf,PETSC_MAX_PATH_LEN-2,file);
  while (buf[0] == '%');
  sscanf(buf,"%d\n",&m);
  for (i = 0; i < m; i++) {
    double v;
    int posx;
    ierr = fscanf(
        file,"%d %le\n",
        &posx,(double*)&v);
    // read value from file
    // XXX is it  a good idea to se to the value of a local variable?
    ierr = VecSetValues(b, 1, &i, &v, ADD_VALUES);CHKERRQ(ierr);
  }
  fclose(file);

  // --- Solve system
  KSP ksp;
  ierr = KSPCreate(PETSC_COMM_WORLD,&ksp);CHKERRQ(ierr);
  ierr = KSPSetOperators(ksp,A,A);CHKERRQ(ierr);
  ierr = KSPSetFromOptions(ksp);CHKERRQ(ierr);
  ierr = KSPSolve(ksp,b,x);CHKERRQ(ierr);
  ierr = KSPGetIterationNumber(ksp,&its);CHKERRQ(ierr);

  // --- Check solution
  ierr = MatMult(A,x,u);CHKERRQ(ierr);
  ierr = VecAXPY(u,-1.0,b);CHKERRQ(ierr);
  PetscReal norm;
  ierr = VecNorm(u,NORM_2,&norm);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Norm of error %f, Iterations %d\n",(double)norm, its);CHKERRQ(ierr);
  // VecView(x, PETSC_VIEWER_STDOUT_SELF);

  // --- Clean up
  ierr = VecDestroy(&x);CHKERRQ(ierr); ierr = VecDestroy(&u);CHKERRQ(ierr);
  ierr = VecDestroy(&b);CHKERRQ(ierr); ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = KSPDestroy(&ksp);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
