#include <string>
#include <string.h>
#include "phonenumberutil.h"

extern "C" {
    #include "postgres.h"
    #include "fmgr.h"
    #include "utils/array.h"
    #include "catalog/pg_type.h"
}

using namespace std;
using namespace i18n::phonenumbers;
static const PhoneNumberUtil* const phoneUtil = PhoneNumberUtil::GetInstance();

struct phonenumber {
    char *rawinput;
    char *parsed;
    char region[3];
};
typedef phonenumber phonenumber_t;

string ErrorTypeStr[] = {
    "No Parsing Error",
    "Invalid Country Code",
    "Not a number",
    "Too short after international direct dialing prefix",
    "National number is too short",
    "National number is too long",
};

bool is_valid_region(string region) {
	set<string> regions;
	phoneUtil->GetSupportedRegions(&regions);

	return regions.find(region) != regions.end();
}

string decodetypmod(int32 typmod) {
	string region;

	if (typmod > 1) {
		char code[3] = {0};
		code[0] = typmod >> 8;
		code[1] = typmod & 0xFF;
		code[2] = '\0';
		region = string(code);
	}

	return region;
} 

extern "C" {
    PG_MODULE_MAGIC;


    PG_FUNCTION_INFO_V1(phonein);
    Datum phonein(PG_FUNCTION_ARGS) {
        char            *arg1 = PG_GETARG_CSTRING(0);
        int32           typmod = PG_GETARG_INT32(2);
        string          phone(arg1);
		string 			region("US");
        PhoneNumber     phoneNumber;
        string formattedNumber;

		if (typmod > 1) {
			region = decodetypmod(typmod);
		}

        PhoneNumberUtil::ErrorType error = phoneUtil->Parse(phone, region, &phoneNumber);

        if (error) {
            ereport(ERROR, 
                errcode(ERRCODE_INVALID_PARAMETER_VALUE), 
                errmsg("%s '%s'", 
                    ErrorTypeStr[error].data(), arg1));
            PG_RETURN_NULL();
        } else {
            phonenumber_t     *result;
            phoneUtil->Format(phoneNumber, PhoneNumberUtil::E164, &formattedNumber);
            result = (phonenumber_t *)palloc(sizeof(phonenumber_t));
            result->rawinput = pstrdup(arg1);
            result->parsed =pstrdup(formattedNumber.data());
            strncpy(result->region, region.data(), 2);
            result->region[2] = '\0';

            PG_RETURN_POINTER(result);
        }
    }


    PG_FUNCTION_INFO_V1(phoneout);
    Datum phoneout(PG_FUNCTION_ARGS) {
        phonenumber_t   *arg = (phonenumber_t *)PG_GETARG_POINTER(0);
        char *result    = pstrdup(arg->parsed);

        PG_RETURN_CSTRING(result);
    }


    PG_FUNCTION_INFO_V1(phonemodin);
    Datum phonemodin(PG_FUNCTION_ARGS) {
        ArrayType 		*ta = PG_GETARG_ARRAYTYPE_P(0);
        int32     		typmod;
        Datum     		*values;
        bool      		*nullsp;
        int       		nelemsp;

        deconstruct_array(ta, CSTRINGOID, 2, false, 'i', &values, &nullsp, &nelemsp);

        if (nelemsp > 0) {
            char *region = DatumGetCString(values[0]);
            if (strlen(region) == 2 && is_valid_region(region)) {
                typmod = (region[0] << 8) | region[1];
            } else {
                ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("Invalid phone modifier must be a 2 character valid region")));
            }
        } else {
            ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid INTERVAL type modifier")));                 
        }

        PG_RETURN_INT32(typmod);
    }


    PG_FUNCTION_INFO_V1(phonemodout);
    Datum phonemodout(PG_FUNCTION_ARGS) {
		int32 		typmod = PG_GETARG_INT32(0);
        char        *result;

		if (typmod > 1) {
			string region = decodetypmod(typmod);
            result = psprintf("(%s)", region.data());
			PG_RETURN_CSTRING(result);
		} else {
        	PG_RETURN_CSTRING("");
		}
    }

    /*enforce typmod*/
    PG_FUNCTION_INFO_V1(phone_enforce_typmod);
    Datum phone_enforce_typmod(PG_FUNCTION_ARGS) {
        phonenumber_t   *arg1 = (phonenumber_t *)PG_GETARG_POINTER(0);
        int32           typmod = PG_GETARG_INT32(1);
		string 			region;
        PhoneNumber     phoneNumber;
        phonenumber_t   *result = arg1;
        string formattedNumber;

		if (typmod > 0) {
			region = decodetypmod(typmod);
            PhoneNumberUtil::ErrorType error = phoneUtil->Parse(string(arg1->rawinput), region, &phoneNumber);

            if (error) {
                ereport(ERROR, 
                    errcode(ERRCODE_INVALID_PARAMETER_VALUE), 
                    errmsg("%s", 
                        ErrorTypeStr[error].data()));
                PG_RETURN_NULL();
		    }

            phoneUtil->Format(phoneNumber, PhoneNumberUtil::E164, &formattedNumber);
            result = (phonenumber_t *)palloc(sizeof(phonenumber_t));
            result->rawinput = pstrdup(arg1->rawinput);
            result->parsed = pstrdup(formattedNumber.data());
            strncpy(result->region, region.data(), 2);
            result->region[2] = '\0';
        }
        elog(NOTICE,"%s", result->parsed);

        PG_RETURN_POINTER(result);
    }
    

    PG_FUNCTION_INFO_V1(phonenumber_eq);
    Datum phonenumber_eq(PG_FUNCTION_ARGS) {
        phonenumber_t   *arg1 = (phonenumber_t *)PG_GETARG_POINTER(0);
        phonenumber_t   *arg2 = (phonenumber_t *)PG_GETARG_POINTER(1);

        PG_RETURN_BOOL(!strcasecmp(arg1->parsed, arg2->parsed));
    }


    PG_FUNCTION_INFO_V1(phonenumber_ne);
    Datum phonenumber_ne(PG_FUNCTION_ARGS) {
        phonenumber_t   *arg1 = (phonenumber_t *)PG_GETARG_POINTER(0);
        phonenumber_t   *arg2 = (phonenumber_t *)PG_GETARG_POINTER(1);

        PG_RETURN_BOOL(strcasecmp(arg1->parsed, arg2->parsed));
    }
};