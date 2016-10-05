#include <petscksp.h>

void readVector(Vec* x, int m, const char* optionName) {
  char vfilein[PETSC_MAX_PATH_LEN], buf[PETSC_MAX_PATH_LEN];

  PetscBool found;
  PetscOptionsGetString(NULL, NULL, optionName, vfilein, PETSC_MAX_PATH_LEN, &found);
  FILE *file;
  PetscFOpen(PETSC_COMM_SELF, vfilein, "r", &file); 

  // read header
  const char* supportedFormat = "%%MatrixMarket matrix array real general\n";
  fgets(buf, PETSC_MAX_PATH_LEN, file);
  if (strncmp(buf, supportedFormat, PETSC_MAX_PATH_LEN) != 0) {
    printf("Only supporting: '%s', got '%s'", supportedFormat, buf);
    PetscFClose(PETSC_COMM_SELF, file);
    return;
  }

  VecCreate(PETSC_COMM_WORLD, x);
  PetscObjectSetName((PetscObject) *x, optionName);
  VecSetSizes(*x, PETSC_DECIDE, m);
  VecSetFromOptions(*x);

  // skip comments
  do
    fgets(buf, PETSC_MAX_PATH_LEN-2, file);
  while (buf[0] == '%');

  sscanf(buf,"%d\n",&m);
  int i;
  for (i = 0; i < m; i++) {
    double v;
    fscanf(file,"%lf", (double*)&v);
    VecSetValues(*x, 1, &i, &v, ADD_VALUES);
  }

  PetscFClose(PETSC_COMM_SELF, file);
}

int main(int argc,char **args) {
  Mat            A;
  PetscErrorCode ierr;
  PetscInt       i,n,its;
  PetscMPIInt    size;
  char           filein[PETSC_MAX_PATH_LEN], buf[PETSC_MAX_PATH_LEN];
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
  ierr = PetscPrintf (PETSC_COMM_SELF,"m = %d, n = %d, nnz = %d\n",m,n,nnz); CHKERRQ(ierr);
  ierr = PetscMalloc4(nnz,&Is,nnz,&Js,nnz,&VALs,m,&rownz);CHKERRQ(ierr); 

  for (i=0; i<nnz; i++) {
    ierr = fscanf(file,"%d %d %le\n",&Is[i],&Js[i],(double*)&VALs[i]);
    if (ierr == EOF) {
      printf("Warning!! Reached EOF, %d\n", i);
      nnz = i;
      break;
    }
    Is[i]--;
    Js[i]--;
  }
  fclose(file);

  ierr = MatCreate(PETSC_COMM_SELF,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,m,n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSeqSBAIJSetPreallocation(A,1,0,rownz);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  for (i=0; i<nnz; i++) {
    MatSetValues(A,1,&Is[i],1,&Js[i],&VALs[i],INSERT_VALUES);
    MatSetValues(A,1,&Js[i],1,&Is[i],&VALs[i],INSERT_VALUES);
  }

  MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);
  MatSetOption(A, MAT_SPD, PETSC_TRUE);
  PetscPrintf(PETSC_COMM_SELF,"Matrix assembled\n");

  // --- Load RHS
  Vec x, b, u;
  readVector(&b, m, "-vin");
  VecView(b, PETSC_VIEWER_STDOUT_SELF);
  VecDuplicate(b,&x);
  VecDuplicate(b,&u);

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
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Norm of error %e, Iterations %d\n", norm, its);CHKERRQ(ierr);

  // --- Load exp solution and check norm
  Vec exp;
  readVector(&exp, m, "-ein");
  PetscPrintf(PETSC_COMM_SELF,"==> Computed solution\n");
  VecView(x, PETSC_VIEWER_STDOUT_SELF);
  PetscPrintf(PETSC_COMM_SELF,"==> Expected solution\n");
  VecView(exp, PETSC_VIEWER_STDOUT_SELF);
  VecAXPY(x,-1.0,exp);
  VecNorm(x,NORM_2,&norm);
  PetscPrintf(PETSC_COMM_SELF,"L2 norm: %lf\n", (double)norm);

  // --- Clean up
  VecDestroy(&x);
  VecDestroy(&u);
  VecDestroy(&b);
  VecDestroy(&exp);
  MatDestroy(&A);
  KSPDestroy(&ksp);
  PetscFinalize();
  return 0;
}
