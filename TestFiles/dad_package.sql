BEGIN
  -- drag & drop
  Dbms_Stats.gather_database_stats(estimate_percent => , block_sample => , method_opt => , degree => , granularity => , cascade => , stattab => , statid => , options => , objlist => , statown => , gather_sys => , no_invalidate => , gather_temp => , gather_fixed => , stattype => );

  -- drag & drop with pressed <>
  Dbms_Stats.gather_database_stats
  (
    estimate_percent => ,
    block_sample     => ,
    method_opt       => ,
    degree           => ,
    granularity      => ,
    cascade          => ,
    stattab          => ,
    statid           => ,
    options          => ,
    objlist          => ,
    statown          => ,
    gather_sys       => ,
    no_invalidate    => ,
    gather_temp      => ,
    gather_fixed     => ,
    stattype         =>
  );
END;
/
