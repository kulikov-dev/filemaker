### Connector to the relational database [FileMaker](https://help.claris.com/en/data-api-guide/content/index.html) for PHP 5.5 >=

The connector allows to communicate with the FileMaker tables through the REST API, realizing operations of obtaining, finding and updating data. Support scripts.

Before work you need to initialize a connector with your credentials:
``` php
    private static function initialize_fileMaker()
    {
        self::$file_maker_connector = new filemaker_connector();
        self::$file_maker_connector->set_credentials('login', 'pass');
        self::$file_maker_connector->set_entry_point('https://a111111.fmphost.com/fmi/data/v1/databases/test');
        self::$file_maker_connector->open_connection();

        if (self::$file_maker_connector == null || !self::$file_maker_connector->is_opened()) {
            die('Initialize FileMakerConnector before work with Test database.');
        }
    }
```

After that the connector is ready to work. Below are samples of obtaining data with a FileMaker script:
``` php
 $script = '&script=FindActiveForModifiedOnly&script.param=' . date("m-d-Y H:i:s", time());
 $indexer = self::$file_maker_connector->get_table_content('Inventory', $script);
```

And updating a record in the FileMaker database:
``` php
  $new_record = [];
  $new_record["pk"] = strval($employee_info["WOVEN_ID"]);

  $new_record["first"] = $employee_info["FNAME"];
  $new_record["last"] = $employee_info["LNAME"];
  $query_info = [
            "query" => [[
                "Employee" => '=' . $new_record["pk"]
            ]],
            "limit" => "1",
            "script" => "Employee DataAPIUpdate",
            "script.param" => json_encode($new_record)
        ];

  self::$file_maker_connector->update_record("Employees", $query_info);
```
