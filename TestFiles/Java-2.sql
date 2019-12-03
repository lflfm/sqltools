CREATE JAVA SOURCE NAMED "Welcome" AS
   public class Welcome {
      public static String welcome() {
         return "Welcome World";   } }
/

CREATE AND COMPILE JAVA SOURCE NAMED "Welcome" AS
   public class Welcome {
      public static String welcome() {
         return "Welcome World";   } }
/

CREATE OR REPLACE JAVA SOURCE NAMED "Welcome" AS
   public class Welcome {
      public static String welcome() {
         return "Welcome World";   } }
/

CREATE OR REPLACE AND COMPILE JAVA SOURCE NAMED "Welcome" AS
   public class Welcome {
      public static String welcome() {
         unknown();
         return "Welcome World";   } }
/

ALTER JAVA SOURCE "Welcome" COMPILE;
ALTER JAVA CLASS "Welcome" COMPILE;

SELECT * FROM user_errors WHERE name = 'Welcome' AND TYPE = 'JAVA SOURCE' ORDER BY sequence;
SELECT * FROM user_objects WHERE object_name = 'Welcome' AND object_type LIKE 'JAVA%';
SELECT * FROM user_source WHERE name = 'Welcome';


CREATE OR REPLACE AND COMPILE JAVA SOURCE NAMED "FileHandler" AS
/*test*/
    import java.lang.*;
    import java.util.*;
    import java.io.*;
    import java.sql.Timestamp;

    public class FileHandler
    {
        private static int SUCCESS = 1;
        private static  int FAILURE = 0;
//
        public static int isFile (String path) {
            File myFile = new File (path);
            if (myFile.isFile()) return SUCCESS; else return FAILURE;
        }

        public static int isHidden (String path) {
            File myFile = new File (path);
            if (myFile.isHidden()) return SUCCESS; else return FAILURE;
        }

    }
/


CREATE OR REPLACE AND COMPILE JAVA SOURCE NAMED "UpdateCar" AS
import java.sql.*;

public class UpdateCar {

    public static void UpdateCarNum(int carNo, int empNo) //throws SQLException
    {

        Connection con = null;
        PreparedStatement pstmt = null;

        try {
            con = DriverManager.getConnection(
                      "jdbc:default:connection");

            pstmt = con.prepareStatement(
            get_something()
                        "UPDATE EMPLOYEES " +
                        "SET CAR_NUMBER = ? " +
                        "WHERE EMPLOYEE_NUMBER = ?");

            pstmt.setInt(1, carNo);
            pstmt.setInt(2, empNo);
            pstmt.executeUpdate();
        }
        finally {
            if (pstmt != null) pstmt.close();
        }
    }
}
/

SELECT * FROM user_errors WHERE name = 'UpdateCar';