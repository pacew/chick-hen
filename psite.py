import json
import os
import sqlite3

cfg = None
options = None

def read_json(name, default=None):
   try:
      with open(name) as f:
         val = json.load (f)
   except(OSError, ValueError):
      if default is not None:
         return default
      print ("Can't parse", name)
      sys.exit(1)
   return val

def write_json(name, val):
   with open("TMP.json", "w") as f:
      f.write(json.dumps(val, sort_keys=True, indent=2))
      f.write("\n")
   os.rename("TMP.json", name)
      
def slurp_file(name):
   try:
      with open(name) as f:
         val = f.read()
   except(OSError):
      val = ""
   return val

def get_cfg():
   global cfg
   if cfg is None:
      cfg = read_json("cfg.json")
   return cfg

def get_options():
   global options
   if options is None:
      options =read_json("options.json")
   return options

db = None

def get_db():
   global db

   if db is not None:
      return db
   
   cfg = get_cfg()
   if cfg["db"] == "sqlite3":
      filename = "{0}.db".format(cfg['site_name'])
      db = {}
      db['conn'] = sqlite3.connect(filename)
      db['cursor'] = db['conn'].cursor()
      db['make_column'] = sqlite3_make_column
      return db
   else:
      print("get_db failed")
      sys.exit(1)

def make_column(table, column, coltype):
   db = get_db()
   return db['make_column'](table, column, coltype)

def sqlite3_table_exists(table):
   db = get_db()
   cur = db['cursor']
   cur.execute("select 1 from sqlite_master"+
               " where type = 'table'"+
               "   and name = ?",
               (table,))
   return cur.fetchone() != None

def sqlite3_column_exists(table, column):
   db = get_db()
   cur = db['cursor']
   stmt = "pragma table_info({0})".format(table)
   cur.execute(stmt)
   for row in cur:
      if row[1] == column:
         return True
   return False

def sqlite3_make_column(table, column, typename):
   global db
   if not sqlite3_table_exists(table):
      stmt = "create table {0} ({1} {2})".format(table, column, typename)
      print(stmt)
      db['cursor'].execute(stmt)
   elif not sqlite3_column_exists(table, column):
      stmt = "alter table {0} add {1} {2}".format(table, column, typename)
      print(stmt)
      db['cursor'].execute(stmt)

