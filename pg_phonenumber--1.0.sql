-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION phonenumber" to load this file. \quit

CREATE TYPE phone;

--
--  Input and output functions.
--
CREATE OR REPLACE FUNCTION phone_in(cstring, oid, integer)
RETURNS phone
AS 'MODULE_PATHNAME', 'phone_in'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION phone_out(phone)
RETURNS cstring
AS 'MODULE_PATHNAME', 'phone_out'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

--
--  The type itself.
--

CREATE TYPE phone (
    INPUT          = phone_in,
    OUTPUT         = phone_out,
    INTERNALLENGTH = 16,
    STORAGE        = plain,
    CATEGORY       = 'S',
    PREFERRED      = false
);

--
-- Operator Functions.
--

CREATE FUNCTION phone_eq( phone, phone )
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION phone_ne( phone, phone )
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

--
-- Operators.
--

CREATE OPERATOR = (
    LEFTARG    = phone,
    RIGHTARG   = phone,
    COMMUTATOR = =,
    NEGATOR    = <>,
    PROCEDURE  = phone_eq,
    RESTRICT   = eqsel,
    JOIN       = eqjoinsel,
    HASHES = true,
    MERGES = true
);

CREATE OPERATOR <> (
    LEFTARG     = phone,
    RIGHTARG    = phone,
    COMMUTATOR  = <>,
    NEGATOR     = =,
    PROCEDURE   = phone_ne,
    RESTRICT    = eqsel,
    JOIN        = eqjoinsel,
    HASHES      = true,
    MERGES      = true
);

CREATE FUNCTION get_supported_regions()
RETURNS TEXT[]
AS 'MODULE_PATHNAME', 'get_supported_regions'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE FUNCTION get_supported_calling_codes()
RETURNS INT[]
AS 'MODULE_PATHNAME', 'get_supported_calling_codes'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;