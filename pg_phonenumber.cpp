#include <string>
#include <string.h>
#include "phonenumberutil.h"

extern "C" {
    #include "postgres.h"
    #include "fmgr.h"
    #include "utils/array.h"
    #include "catalog/pg_type.h"
    #include "utils/guc.h"
    #include "utils/builtins.h"
}

using namespace std;
using namespace i18n::phonenumbers;
static const PhoneNumberUtil* const phoneUtil = PhoneNumberUtil::GetInstance();

char* default_phone_region = NULL;

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

bool guc_region_checkhook(char **newval, void **extra, GucSource source) {
    return is_valid_region(string(*newval));
}


extern "C" {
    PG_MODULE_MAGIC;

    void _PG_init(void) {
        DefineCustomStringVariable(
            "pg_phone.default_region",  // GUC name
            "Default region for phone numbers",  // Description
            NULL,  // Long description
            &default_phone_region,  // Pointer to the default value
            "US",  // Default value
            PGC_USERSET,  // Scope: user-settable
            0,  // Flags
            guc_region_checkhook,  // Check hook
            NULL,  // Assign hook
            NULL   // Show hook
        );

        MarkGUCPrefixReserved("pg_phone");
    }


    PG_FUNCTION_INFO_V1(phone_in);
    Datum phone_in(PG_FUNCTION_ARGS) {
        char            *arg = PG_GETARG_CSTRING(0);
        string          phone(arg);
		string 			region("US");
        PhoneNumber     phoneNumber;
        string formattedNumber;

		if (default_phone_region != NULL) {
            region = string(default_phone_region);
        }

        PhoneNumberUtil::ErrorType error = phoneUtil->Parse(phone, region, &phoneNumber);

        if (error) {
            ereport(ERROR, 
                errcode(ERRCODE_INVALID_PARAMETER_VALUE), 
                errmsg("%s '%s'", 
                    ErrorTypeStr[error].data(), arg));
        } else if (!phoneUtil->IsValidNumber(phoneNumber)) {
            ereport(ERROR,
                errcode(ERRCODE_INVALID_PARAMETER_VALUE), 
                errmsg("Invalid number"));
        } else {
            char     *result;
            phoneUtil->Format(phoneNumber, PhoneNumberUtil::E164, &formattedNumber);
            result = (char *)palloc(sizeof formattedNumber.data()+1);
            result = pstrdup(formattedNumber.data());

            PG_RETURN_POINTER(result);
        }
    }


    PG_FUNCTION_INFO_V1(phone_out);
    Datum phone_out(PG_FUNCTION_ARGS) {
        char   *arg = (char *)PG_GETARG_CSTRING(0);
        char *result    = pstrdup(arg);

        PG_RETURN_CSTRING(result);
    }


    PG_FUNCTION_INFO_V1(phone_eq);
    Datum phone_eq(PG_FUNCTION_ARGS) {
        char   *arg1 = (char *)PG_GETARG_CSTRING(0);
        char   *arg2 = (char *)PG_GETARG_CSTRING(1);

        PG_RETURN_BOOL(!strcasecmp(arg1, arg2));
    }


    PG_FUNCTION_INFO_V1(phone_ne);
    Datum phone_ne(PG_FUNCTION_ARGS) {
        char   *arg1 = (char *)PG_GETARG_CSTRING(0);
        char   *arg2 = (char *)PG_GETARG_CSTRING(1);

        PG_RETURN_BOOL(strcasecmp(arg1, arg2));
    }


    PG_FUNCTION_INFO_V1(get_supported_regions);
    Datum get_supported_regions(PG_FUNCTION_ARGS) {
	    set<string> regions;
        phoneUtil->GetSupportedRegions(&regions);
        int region_count = regions.size();
        Datum *region_datums = (Datum *) palloc(sizeof(Datum) * region_count);

        int i = 0;
        for (const auto &region : regions) {
            region_datums[i++] = CStringGetTextDatum(region.data());
        }

        ArrayType *result_array = construct_array_builtin(region_datums, region_count, TEXTOID);
        PG_RETURN_ARRAYTYPE_P(result_array);
    }


    PG_FUNCTION_INFO_V1(get_supported_calling_codes);
    Datum get_supported_calling_codes(PG_FUNCTION_ARGS) {
	    set<int> calling_codes;
        phoneUtil->GetSupportedCallingCodes(&calling_codes);
        int count = calling_codes.size();
        Datum *datums = (Datum *) palloc(sizeof(Datum) * count);

        int i = 0;
        for (const auto &calling_code : calling_codes) {
            datums[i++] = Int32GetDatum(calling_code);
        }

        ArrayType *result_array = construct_array_builtin(datums, count, INT4OID);
        PG_RETURN_ARRAYTYPE_P(result_array);
    }
};