CREATE TABLE compound_trigger_test (
  id           NUMBER,
  description  VARCHAR2(50)
);

CREATE OR REPLACE TRIGGER compound_trigger_test_trg
  FOR INSERT OR UPDATE OR DELETE ON compound_trigger_test
COMPOUND TRIGGER

  -- Global declaration.
  TYPE t_tab IS TABLE OF VARCHAR2(50);
  l_tab t_tab := t_tab();

  BEFORE STATEMENT IS
  BEGIN
    l_tab.extend;
    CASE
      WHEN INSERTING THEN
        l_tab(l_tab.last) := 'BEFORE STATEMENT - INSERT';
      WHEN UPDATING THEN
        l_tab(l_tab.last) := 'BEFORE STATEMENT - UPDATE';
      WHEN DELETING THEN
        l_tab(l_tab.last) := 'BEFORE STATEMENT - DELETE';
    END CASE;
  END BEFORE STATEMENT;

  BEFORE EACH ROW IS
  BEGIN
    l_tab.extend;
    CASE
      WHEN INSERTING THEN
        l_tab(l_tab.last) := 'BEFORE EACH ROW - INSERT (new.id=' || :new.id || ')';
      WHEN UPDATING THEN
        l_tab(l_tab.last) := 'BEFORE EACH ROW - UPDATE (new.id=' || :new.id || ' old.id=' || :old.id || ')';
      WHEN DELETING THEN
        l_tab(l_tab.last) := 'BEFORE EACH ROW - DELETE (old.id=' || :old.id || ')';
    END CASE;
  END BEFORE EACH ROW;

  AFTER EACH ROW IS
  BEGIN
    l_tab.extend;
    CASE
      WHEN INSERTING THEN
        l_tab(l_tab.last) := 'AFTER EACH ROW - INSERT (new.id=' || :new.id || ')';
      WHEN UPDATING THEN
        l_tab(l_tab.last) := 'AFTER EACH ROW - UPDATE (new.id=' || :new.id || ' old.id=' || :old.id || ')';
      WHEN DELETING THEN
        l_tab(l_tab.last) := 'AFTER EACH ROW - DELETE (old.id=' || :old.id || ')';
    END CASE;
  END AFTER EACH ROW;

  AFTER STATEMENT IS
  BEGIN
    l_tab.extend;
    CASE
      WHEN INSERTING THEN
        l_tab(l_tab.last) := 'AFTER STATEMENT - INSERT';
      WHEN UPDATING THEN
        l_tab(l_tab.last) := 'AFTER STATEMENT - UPDATE';
      WHEN DELETING THEN
        l_tab(l_tab.last) := 'AFTER STATEMENT - DELETE';
    END CASE;

    FOR i IN l_tab.first .. l_tab.last LOOP
      DBMS_OUTPUT.put_line(l_tab(i));
    END LOOP;
    l_tab.delete;
  END AFTER STATEMENT;

END compound_trigger_test_trg;
/