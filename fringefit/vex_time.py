#!/usr/bin/env python
import datetime
import re

def get_time(vextime):
  year, yday, hour, minute, sec = [int(x) for x in re.split('[y|d|h|m|s]', vextime)[:-1]]
  seconds = 3600 * hour + 60 * minute + sec
  t0 = datetime.datetime(year, 1, 1)
  dt = datetime.timedelta(yday - 1, seconds)
  return t0 + dt

def get_date_string(year, day, seconds):
  hour = seconds // (60*60)
  minute = (seconds % (60*60))//60
  sec = seconds % 60
  return "%dy%03dd%02dh%02dm%02ds"%(year, day, hour, minute, sec)
