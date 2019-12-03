CONNECT sys
DROP DIRECTORY bfile_test_dir;
CREATE DIRECTORY bfile_test_dir AS 'c:\Temp';
GRANT read, write ON DIRECTORY bfile_test_dir TO scott;

CONNECT scott
CREATE TABLE bfile_test (id NUMBER, bf BFILE);
DELETE FROM bfile_test;
INSERT INTO bfile_test VALUES (1, BFILENAME('BFILE_TEST_DIR', 'license.txt'));
INSERT INTO bfile_test VALUES (2, BFILENAME('bfile_test_dir', 'license.txt'));
COMMIT;

SELECT id, bf FROM bfile_test;

/*
void BfileGetDirFile(envhp, errhp, svchp, stmthp)
OCIEnv       *envhp;
OCIError     *errhp;
OCISvcCtx    *svchp;
OCIStmt      *stmthp;
{

   OCILobLocator *bfile_loc;
   OraText dir_alias[32] = NULL;
   OraText filename[256]  = NULL;
   ub2 d_length = 32;
   ub2 f_length = 256;

   /* Allocate the locator descriptors */
   (void) OCIDescriptorAlloc((dvoid *) envhp, (dvoid **) &bfile_loc,
                             (ub4) OCI_DTYPE_FILE,
                             (size_t) 0, (dvoid **) 0);

   /* Select the bfile */
   selectLob(bfile_loc, errhp, svchp, stmthp);

   checkerr(errhp, OCILobFileGetName(envhp, errhp, bfile_loc,
                       dir_alias, &d_length, filename, &f_length));

   printf("Directory Alias : [%s]\n", dir_alias);
   printf("File name : [%s]\n", filename);

   /* Free the locator descriptor */
   OCIDescriptorFree((dvoid *)bfile_loc, (ub4)OCI_DTYPE_FILE);
}
*/