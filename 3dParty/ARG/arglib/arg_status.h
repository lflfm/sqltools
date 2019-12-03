// This is a rough draft - hence inlined code etc.

#include <string>

namespace arg
{
    using std::string;
    class GenericErrorStatusFactory;


    /// Encapsulation of return status
    class Status
    {
    public:
        enum Code
        {
            Success,                // The operation was a success
            TemporaryError,         // Retrying the operation might work
            PermanentError,         // Retrying the operation won't work
            ContractViolationError, // Don't be stupid!
        };

        /// Default constructor - corresponds to "success" status code
        Status();

        /// Copy constructor
        Status(const Status&);

        /// Assignment
        Status& operator=(const Status&);
        
        /// Converts to "true" if (and only) if success
        operator bool() const               { return Success == status_code; }
        
        /// Conversion to string
        string getErrorString() const       { return error_string; }

    private:

        friend GenericErrorStatusFactory;
        
        /// Error constructor - only accessed by GenericErrorStatusFactory
        Status(Code status, string error);

        Code    status_code;
        string  error_string;
    };    

    class GenericErrorStatusFactory
    {
    protected:
        Status createStatus(Status::Code status, string error) const
        {
            return Status(status, error);
        }
    };
    
    
    template<typename Ret_t>
    class Fallible
    {
    public:
        typedef Ret_t ReturnType;
    
        /// Construct a successful return
        Fallible(ReturnType retvalue);

        /// Construct an unsuccessful return
        Fallible(Status status);
        
        ///
        Status      exit_status;
        
        ///
        ReturnType  value;
    };
}

