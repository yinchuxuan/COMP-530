
(1) 
SELECT
        n.n_name,
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l,
        supplier AS s,
        nation AS n,
        region AS r
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey)
        AND (l.l_suppkey = s.s_suppkey)
        AND (c.c_nationkey = s.s_nationkey)
        AND (s.s_nationkey = n.n_nationkey)
        AND (n.n_regionkey = r.r_regionkey)
        AND (r.r_name = "region")
        AND (o.o_orderdate > "date1" OR o.o_orderdate = "date1")
        AND (NOT o.o_orderdate < "date2")
GROUP BY
        n.n_name;

(2)
SELECT 
	n.n_name,
	s.s_suppkey,
	SUM(l.l_extendedprice)
FROM 
	nation AS n,
	supplier AS s,
	lineitem AS l
WHERE 
	(l.l_suppkey = s.s_suppkey)
	AND (s.s_nationkey = n.n_nationkey)
GROUP BY
	n.n_name;

(3)
SELECT
	SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
	lineitem AS l
WHERE
	(l.l_extendedprice * (1 - l.l_discount) > "a string");

(4)
SELECT
	SUM(l.l_returnflag + l.l_shipdate + l.l_commitdate)
FROM 
	lineitem AS l;

(5)
SELECT
	l.l_returnflag + l.l_shipdate + l.l_commitdate
FROM 
	lineitem AS l
WHERE
	(l.l_shipdate > "a date"); 

(6)
SELECT
        n.n_name,
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l,
        supplier AS s,
        nation AS n,
        region AS r
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey)
        AND (l.l_suppkey = s.s_suppkey)
        AND (c.c_nationkey = s.s_nationkey)
        AND (s.s_nationkey = n.n_nationkey)
        AND (n.n_regionkey = r.r_regionkey)
        AND (r.r_name = "region")
        AND (l.l_tax + l.l_discount > "date2" OR o.o_orderdate = "date1")
        AND (NOT o.o_orderdate < "date2")
GROUP BY
        n.n_name;
	
(7)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.k_orderkey);

(8)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = h.o_custkey)
        AND (l.l_orderkey = o.o_orderkey);

(9)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND ("this is a string" + "this is another string" > 15.0 - "here is another")
        AND (l.l_orderkey = o.o_orderkey);

(10)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customers AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey);


(11)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey)
	AND (1200 + l.l_quantity / (300.0 + 34) = 33);

(12)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND ("this is a string" + "this is another string" > "here is another")
        AND (l.l_orderkey = o.o_orderkey);

(13)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount)),
	SUM(c.l_extendedprice * 1.01)
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
        AND (l.l_orderkey = o.o_orderkey);

(14)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND ("1204" + "this is a string" = l.l_returnflag)
	AND (l.l_tax + l.l_discount + l.l_extendedprice > 3.27)
        AND (l.l_orderkey = o.o_orderkey);


(15)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount))
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND (l.l_tax + l.l_discount + l.l_extendedprice > 3.27 OR 
		l.l_discount + l.l_extendedprice > "327")
        AND (l.l_orderkey = o.o_orderkey);


(16)
SELECT
        SUM(l.l_extendedprice * (1 - l.l_discount)),
	SUM(1 - l.l_discount)
FROM
        customer AS c,
        orders AS o,
        lineitem AS l
WHERE
        (c.c_custkey = o.o_custkey)
	AND (l.l_tax + l.l_discount + l.l_extendedprice > 3.27 OR 
		"here is a string" > "327")
        AND (l.l_orderkey = o.o_orderkey);

(17)
SELECT
	SUM(12 + 13.4 + 19),
	SUM(l.l_extendedprice)
FROM 
	lineitem AS l;


