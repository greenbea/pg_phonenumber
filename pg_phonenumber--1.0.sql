-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION phonenumber" to load this file. \quit

CREATE TYPE phone;

--
--  Input and output functions.
--
CREATE OR REPLACE FUNCTION phonenumberin(cstring, oid, integer)
RETURNS phone
AS 'MODULE_PATHNAME', 'phonein'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION phonenumberout(phone)
RETURNS cstring
AS 'MODULE_PATHNAME', 'phoneout'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION phonenumbermodin(cstring[])
RETURNS int
AS 'MODULE_PATHNAME', 'phonemodin'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION phonenumbermodout(int)
RETURNS cstring
AS 'MODULE_PATHNAME', 'phonemodout'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION phone(phone, int)
RETURNS phone
AS 'MODULE_PATHNAME', 'phone_enforce_typmod'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

--
--  The type itself.
--

CREATE TYPE phone (
    INPUT          = phonenumberin,
    OUTPUT         = phonenumberout,
    TYPMOD_IN      = phonenumbermodin,
    TYPMOD_OUT     = phonenumbermodout,
    INTERNALLENGTH = 60,
    STORAGE        = plain,
    CATEGORY       = 'S',
    PREFERRED      = false
);

--
--  Casts.
--
CREATE CAST (phone as phone) WITH FUNCTION phone AS IMPLICIT;

--
-- Operator Functions.
--

CREATE FUNCTION phonenumber_eq( phone, phone )
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION phonenumber_ne( phone, phone )
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
    PROCEDURE  = phonenumber_eq,
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
    PROCEDURE   = phonenumber_ne,
    RESTRICT    = eqsel,
    JOIN        = eqjoinsel,
    HASHES      = true,
    MERGES      = true
);