/*-------------------------------------------------------------------------
 *
 * Copyright (c) 2022, Gurjeet Singh
 *
 * IDENTIFICATION
 *	  contrib/pg_plan_guarantee/pg_plan_guarantee.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "optimizer/planner.h"
#include "storage/lmgr.h"
#include "tcop/utility.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

/* Saved hooks */
static planner_hook_type prev_planner_hook = NULL;

/* GUC variables */
static bool pgpg_enabled; /* Is the extension enabled? */

/* Function declarations */
void		_PG_init(void);
void		_PG_fini(void);

static PlannedStmt *pgpg_planner(Query *parse,
								 const char *query_string,
								 int cursorOptions,
								 ParamListInfo boundParams);

static void AcquireExecutorLocks(List *stmt_list, bool acquire);

/*
 * Module load callback
 */
void
_PG_init(void)
{

	DefineCustomBoolVariable("pg_plan_guarantee.enabled",
							 "Enable or disable the pg_plan_guarantee extension",
							 NULL,
							 &pgpg_enabled,
							 true,
							 PGC_USERSET,
							 0,
							 NULL,
							 NULL,
							 NULL);

	MarkGUCPrefixReserved("pg_plan_guarantee");

	/*
	 * Install hooks.
	 */
	prev_planner_hook = planner_hook;
	planner_hook = pgpg_planner;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/* Uninstall hooks. */
	planner_hook = prev_planner_hook;
}

/*
 * pgpg_planner: The workhorse.
 */
static PlannedStmt *
pgpg_planner(Query *parse,
			 const char *query_string,
			 int cursorOptions,
			 ParamListInfo boundParams)
{
	PlannedStmt *result;

	if (pgpg_enabled
		&& parse->commandType == CMD_SELECT
		&& parse->jointree->fromlist == NULL
		&& list_length(parse->targetList) == 2)
		{
			List   *targetList	= parse->targetList;
			char   *res1_name	= (char* )((TargetEntry*)list_nth(targetList,0))->resname;
			char   *res2_name	= (char* )((TargetEntry*)list_nth(targetList,1))->resname;
			int		expr1_type	= ((Expr*)((TargetEntry*)list_nth(targetList,0))->expr)->type;
			int		expr2_type	= ((Expr*)((TargetEntry*)list_nth(targetList,1))->expr)->type;
			Const  *const1		= (Const*)((TargetEntry*)list_nth(targetList,0))->expr;
			Const  *const2		= (Const*)((TargetEntry*)list_nth(targetList,1))->expr;
			Oid		const1_type	= const1->consttype;
			Oid		const2_type	= const2->consttype;
			bool	const1_is_null	= const1->constisnull;
			bool	const2_is_null	= const2->constisnull;

			if (strcmp(res1_name, "query") == 0
				&& strcmp(res2_name, "plan") == 0
				&& expr1_type == T_Const
				&& expr2_type == T_Const
				&& const1_type == TEXTOID
				&& const2_type == TEXTOID
				&& !const1_is_null
				&& !const2_is_null)
			{
                /* The command matches the shape we expect */

				char *query_text = TextDatumGetCString(const1->constvalue);
				char *plan_text = TextDatumGetCString(const2->constvalue);

				elog(DEBUG1, "query: %s", query_text);
				elog(DEBUG1, "plan: %s", plan_text);

				if (strlen(plan_text) == 0)
				{
					/*
					 * User wants us to generate the plan and return it as
					 * result.
					 */

					Datum	plan_text_datum;
					List   *parsetree_list;
					List   *querytree_list;
					RawStmt*raw_parsetree;

					elog(DEBUG1, "plan text length is 0");

					parsetree_list = pg_parse_query(query_text);
					if (list_length(parsetree_list) > 1)
						elog(ERROR, "pg_plan_guarantee: cannot use multiple commands in query text");

					raw_parsetree = linitial_node(RawStmt, parsetree_list);

					querytree_list = pg_analyze_and_rewrite_fixedparams(raw_parsetree, query_text,
												NULL, 0, NULL);

					if (list_length(querytree_list) > 1)
						elog(ERROR, "pg_plan_guarantee: cannot use multiple queries in query text");

					elog(DEBUG1, "querytree_list: %s", nodeToString(querytree_list));

					if (prev_planner_hook)
						result = prev_planner_hook(linitial_node(Query, querytree_list),
													query_text, cursorOptions,
													boundParams);
					else
						result = standard_planner(linitial_node(Query, querytree_list),
													 query_text, cursorOptions,
													boundParams);

					plan_text = nodeToString(result);
					plan_text_datum = CStringGetTextDatum(plan_text);

					elog(DEBUG1, "new plan text: %s", plan_text);

					/* Replace the zero-length string in second column with the guranteed-plan */
					const2->constvalue = plan_text_datum;

					/* Perform the planning, as usual */
					if (prev_planner_hook)
						result = prev_planner_hook(parse, query_string, cursorOptions,
												   boundParams);
					else
						result = standard_planner(parse, query_string, cursorOptions,
												  boundParams);

					return result;
				}
				else
				{
					/*
					 * User wants us to use the guaranteed- plan, and send that to
					 * the executor.
					 */

					Node		*planned_node;
					List		*dummy_list = NIL;
					PlannedStmt	*planned_stmt;

					elog(DEBUG1, "plan text length is NOT 0");

					planned_node = stringToNode(plan_text);

					if (!IsA(planned_node, PlannedStmt))
						elog(ERROR, "pg_plan_guarantee: the provided plan-text is not a PlannedStmt");

					planned_stmt = (PlannedStmt*)planned_node;

					dummy_list = lcons(planned_stmt, dummy_list);

					/* Acquire locks that the Executor assumes are taken the parse-analyze-plan steps. */
					AcquireExecutorLocks(dummy_list, true);

					return planned_stmt;
				}
			}
		}

	/*
	 * If any of the conditions we stipulate did not match, process the Query*
	 * as if the extension was disabled.
	 */
	if (prev_planner_hook)
		result = prev_planner_hook(parse, query_string, cursorOptions,
								   boundParams);
	else
		result = standard_planner(parse, query_string, cursorOptions,
								  boundParams);

   return result;
}

/*
 * AcquireExecutorLocks: acquire locks needed for execution of a cached plan;
 * or release them if acquire is false.
 *
 * Copied verbatim from src/backend/utils/cache/plancache.c. We could use this
 * function from there, instead of duplicating it, if it were exported.
 */
static void
AcquireExecutorLocks(List *stmt_list, bool acquire)
{
	ListCell   *lc1;

	foreach(lc1, stmt_list)
	{
		PlannedStmt *plannedstmt = lfirst_node(PlannedStmt, lc1);
		ListCell   *lc2;
#if 0 // We know for sure we'll not get Utility statments here
		if (plannedstmt->commandType == CMD_UTILITY)
		{
			/*
			 * Ignore utility statements, except those (such as EXPLAIN) that
			 * contain a parsed-but-not-planned query.  Note: it's okay to use
			 * ScanQueryForLocks, even though the query hasn't been through
			 * rule rewriting, because rewriting doesn't change the query
			 * representation.
			 */
			Query	   *query = UtilityContainsQuery(plannedstmt->utilityStmt);

			if (query)
				ScanQueryForLocks(query, acquire);
			continue;
		}
#endif
		foreach(lc2, plannedstmt->rtable)
		{
			RangeTblEntry *rte = (RangeTblEntry *) lfirst(lc2);

			if (rte->rtekind != RTE_RELATION)
				continue;

			/*
			 * Acquire the appropriate type of lock on each relation OID. Note
			 * that we don't actually try to open the rel, and hence will not
			 * fail if it's been dropped entirely --- we'll just transiently
			 * acquire a non-conflicting lock.
			 */
			if (acquire)
				LockRelationOid(rte->relid, rte->rellockmode);
			else
				UnlockRelationOid(rte->relid, rte->rellockmode);
		}
	}
}

