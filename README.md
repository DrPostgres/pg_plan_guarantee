pg_plan_guarantee
=================

Guarantee that your plans will never change.

Overview
--------

You work very hard to tune your queries; to get the exact query execution plan
that satisfies your performance needs.

But some time later, after either the data has changed suffiently, or the
statistics have changed sufficiently, Postgres planner chooses to suddenly pick
a query execution plan that makes your application look unresponsive. This shows
up on your dashboard as an outage. Now you find yourself running against the
clock, to rewrite the query, test it, change the application/ORM, and deploy the
changes, and still just hoping that the production database accepts your
offerings, and executes your query just acceptably enough, so you can go back to
bed.

But if you use the `pg_plan_guarantee` extension, you will never face such a
situation; we can **guarantee** that. This extension provides a mechanism, using
which you can set your execution plan in stone; that is, Postgres will execute
the plan you give it, or it will throw an error, but it will never try to guess
a plan for you.

Because the data and/or statistics change gradually over time, you can be fairly
confident that the guaraneed-plan will continue to serve your needs in a
graceful manner. That is, your guaranted-plan will degrade gracefully, as
opposed to suddenly, at the most inopportune time.

Note: the term `pgpg` in the text below, and in the code, stands for/is an
acronym of the extension's name, `PG_Plan_Guarantee`.

Installation
------------

Place the extension in the `extensions` directory under the Postgres
installation directory. Then add the name of the extension, `pg_plan_guarantee`,
to the `share_preload_libraries` variable in the `postgresql.conf` file.

Disabling the Extension
-----------------------

If you ever wish to disable the extension, you can do so by adding the parameter
`pg_plan_guarantee.enabled`, and setting its value to `false`. This way you
don't really have to remove the extension from the filesystem, or from the
`shared_preload_libraries` parameter.

How to Use
----------

The `pg_plan_guarantee` extension is designed to be used in two steps. In the
first step, you ask the extension to generate the guaranteed-plan. In the second
step, you use ask the extension to use the guaranteed-plan. Please see the
`Demo` section, below, to see these steps in action.

To generate the guaranteed-plan, embed your query in the first string of the
following SQL command:

    select $pgpg$ your query $pgpg$
             as query,
           '' as plan;

The output of the above command will produce the guaranteed-plan in the second
column of the result-set.

To use the guaranteed-plan produded by the first step, you embed the
guaranteed-plan in the second string of the same SQL command, like so:

    select  $pgpg$ your query $pgpg$
                as query,
            $pgpg$ guaranteed-plan $pgpg$
                as plan;

Demo
----


     select $pgpg$select relkind from pg_class where relname = 'pg_class'$pgpg$
                as query,
            $pgpg$$pgpg$
                as plan;
    -[ RECORD 1 ]-----
    query | select relkind from pg_class where relname = 'pg_class'
    plan  | {PLANNEDSTMT :commandType 1 :queryId 0 :hasReturning false :hasModifyingCTE false :canSetTag true :transientPlan false :dependsOnRole false :parallelModeNeeded false :jitFlags 0 :planTree {INDEXSCAN :startup_cost 0.27 :total_cost 8.29 :plan_rows 1 :plan_width 1 :parallel_aware false :parallel_safe true :async_capable false :plan_node_id 0 :targetlist ({TARGETENTRY :expr {VAR :varno 1 :varattno 17 :vartype 18 :vartypmod -1 :varcollid 0 :varlevelsup 0 :varnosyn 1 :varattnosyn 17 :location 7} :resno 1 :resname relkind :ressortgroupref 0 :resorigtbl 1259 :resorigcol 17 :resjunk false}) :qual <> :lefttree <> :righttree <> :initPlan <> :extParam (b) :allParam (b) :scanrelid 1 :indexid 2663 :indexqual ({OPEXPR :opno 93 :opfuncid 62 :opresulttype 16 :opretset false :opcollid 0 :inputcollid 950 :args ({VAR :varno -3 :varattno 1 :vartype 19 :vartypmod -1 :varcollid 950 :varlevelsup 0 :varnosyn 1 :varattnosyn 2 :location 35} {CONST :consttype 19 :consttypmod -1 :constcollid 950 :constlen 64 :constbyval false :constisnull false :location 45 :constvalue 64 [ 112 103 95 99 108 97 115 115 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ]}) :location 43}) :indexqualorig ({OPEXPR :opno 93 :opfuncid 62 :opresulttype 16 :opretset false :opcollid 0 :inputcollid 950 :args ({VAR :varno 1 :varattno 2 :vartype 19 :vartypmod -1 :varcollid 950 :varlevelsup 0 :varnosyn 1 :varattnosyn 2 :location 35} {CONST :consttype 19 :consttypmod -1 :constcollid 950 :constlen 64 :constbyval false :constisnull false :location 45 :constvalue 64 [ 112 103 95 99 108 97 115 115 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ]}) :location 43}) :indexorderby <> :indexorderbyorig <> :indexorderbyops <> :indexorderdir 1} :rtable ({RANGETBLENTRY :alias <> :eref {ALIAS :aliasname pg_class :colnames ("oid" "relname" "relnamespace" "reltype" "reloftype" "relowner" "relam" "relfilenode" "reltablespace" "relpages" "reltuples" "relallvisible" "reltoastrelid" "relhasindex" "relisshared" "relpersistence" "relkind" "relnatts" "relchecks" "relhasrules" "relhastriggers" "relhassubclass" "relrowsecurity" "relforcerowsecurity" "relispopulated" "relreplident" "relispartition" "relrewrite" "relfrozenxid" "relminmxid" "relacl" "reloptions" "relpartbound")} :rtekind 0 :relid 1259 :relkind r :rellockmode 1 :tablesample <> :lateral false :inh false :inFromCl true :requiredPerms 2 :checkAsUser 0 :selectedCols (b 9 24) :insertedCols (b) :updatedCols (b) :extraUpdatedCols (b) :securityQuals <>}) :resultRelations <> :appendRelations <> :subplans <> :rewindPlanIDs (b) :rowMarks <> :relationOids (o 1259) :invalItems <> :paramExecTypes <> :utilityStmt <> :stmt_location 0 :stmt_len 0}

Project Status
--------------


Controversial Idea
------------------

TODO: Show how/why the Postgres developer/hacker community is against such an idea.
