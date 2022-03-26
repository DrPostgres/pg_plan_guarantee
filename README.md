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

Controversial Idea
------------------

TODO: Show how the Postgres developer/hacker community is against such an idea.
