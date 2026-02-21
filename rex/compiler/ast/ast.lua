local kinds = require("compiler.ast.kinds")
local schema = require("compiler.ast.schema")

local ast = {
  kinds = kinds,
  schema = schema,
}

local function shallow_copy(fields)
  local out = {}
  for key, value in pairs(fields or {}) do
    out[key] = value
  end
  return out
end

local function copy_list(src)
  local out = {}
  for i, value in ipairs(src or {}) do
    out[i] = value
  end
  return out
end

local function normalize_param_entry(entry)
  if type(entry) == "string" then
    return entry, nil
  end
  if type(entry) ~= "table" or type(entry.name) ~= "string" then
    return nil, nil
  end
  local bounds = nil
  if entry.bounds and #entry.bounds > 0 then
    bounds = copy_list(entry.bounds)
  end
  return entry.name, bounds
end

function ast.normalize_type_params(raw)
  if not raw then
    return nil, nil
  end

  local names = {}
  local bounds_map = nil
  local entries = raw

  if raw.names then
    entries = raw.names
    if raw.bounds and type(raw.bounds) == "table" then
      bounds_map = shallow_copy(raw.bounds)
    end
  end

  for _, entry in ipairs(entries or {}) do
    local name, bounds = normalize_param_entry(entry)
    if name then
      table.insert(names, name)
      if bounds and #bounds > 0 then
        bounds_map = bounds_map or {}
        bounds_map[name] = bounds
      end
    else
      error("Invalid type parameter entry in AST")
    end
  end

  if #names == 0 then
    names = nil
  end
  if bounds_map and next(bounds_map) == nil then
    bounds_map = nil
  end

  return names, bounds_map
end

function ast.node(kind, fields)
  if type(kind) ~= "string" then
    error("AST kind must be a string")
  end
  if not schema[kind] then
    error("Unknown AST kind: " .. kind)
  end
  local node = shallow_copy(fields)
  node.kind = kind
  return node
end

local function add_error(errors, path, message)
  table.insert(errors, string.format("%s: %s", path, message))
end

local function validate_table(value, path, errors)
  if type(value) ~= "table" then
    return
  end

  if type(value.kind) == "string" then
    local spec = schema[value.kind]
    if not spec then
      add_error(errors, path, "unknown kind '" .. value.kind .. "'")
      return
    end

    for _, field in ipairs(spec.required or {}) do
      if value[field] == nil then
        add_error(errors, path, "missing required field '" .. field .. "' for " .. value.kind)
      end
    end
  end

  for key, child in pairs(value) do
    if type(child) == "table" then
      if type(child.kind) == "string" then
        validate_table(child, path .. "." .. key, errors)
      else
        for i, item in ipairs(child) do
          if type(item) == "table" and type(item.kind) == "string" then
            validate_table(item, string.format("%s.%s[%d]", path, key, i), errors)
          end
        end
      end
    end
  end
end

function ast.validate(root)
  local errors = {}
  validate_table(root, "$", errors)
  if #errors == 0 then
    return true, nil
  end
  return false, errors
end

function ast.assert_valid(root)
  local ok, errors = ast.validate(root)
  if ok then
    return true
  end
  error(table.concat(errors, "\n"))
end

function ast.temporal_value(name, value, lifetime)
  return ast.node("TemporalValue", {
    name = name,
    value = value,
    lifetime = lifetime,
  })
end

function ast.within_block(duration, block)
  return ast.node("WithinBlock", {
    duration = duration,
    block = block,
  })
end

function ast.during_block(condition, block)
  return ast.node("DuringBlock", {
    condition = condition,
    block = block,
  })
end

function ast.ownership_trace(variable, event)
  return ast.node("OwnershipTrace", {
    variable = variable,
    event = event,
  })
end

return ast
