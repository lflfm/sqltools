CREATE OR REPLACE TYPE Complex AS OBJECT (
   rpart REAL,  -- "real" attribute
   ipart REAL,  -- "imaginary" attribute
   MEMBER FUNCTION plus (x Complex) RETURN Complex,  -- method
   MEMBER FUNCTION less (x Complex) RETURN Complex,
   MEMBER FUNCTION times (x Complex) RETURN Complex,
   MEMBER FUNCTION divby (x Complex) RETURN Complex
);
-- should i execute the staement on ; ?
/

CREATE OR REPLACE TYPE BODY Complex AS
   MEMBER FUNCTION plus (x Complex) RETURN Complex IS
   BEGIN
      RETURN Complex(rpart + x.rpart, ipart + x.ipart);
   END plus;

   MEMBER FUNCTION less (x Complex) RETURN Complex IS
   BEGIN
      RETURN Complex(rpart - x.rpart, ipart - x.ipart);
   END less;

   MEMBER FUNCTION times (x Complex) RETURN Complex IS
   BEGIN
      RETURN Complex(rpart * x.rpart - ipart * x.ipart,
                     rpart * x.ipart + ipart * x.rpart);
   END times;

   MEMBER FUNCTION divby (x Complex) RETURN Complex IS
      z REAL := x.rpart**2 + x.ipart**2;
   BEGIN
      RETURN Complex((rpart * x.rpart + ipart * x.ipart) / z,
                     (ipart * x.rpart - rpart * x.ipart) / z);
   END divby;
END;
/