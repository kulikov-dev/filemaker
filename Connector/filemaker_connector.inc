<?php

namespace kulikov_dev\connectors\filemaker;

use Exception;

/**
 * Class FileMakerConnector for working with FileMaker
 * @package kulikov-dev\connectors\filemaker
 */
class filemaker_connector
{
    /**
     * @var int Limit of get all records query. Used to prevent long executed queries
     */
    public static $limit = 500;

    /**
     * @var string Default entry point
     */
    private $entry_point = null;

    /**
     * @var string User name for FileMaker account
     */
    private $user = null;

    /**
     * @var string Password for FileMaker account
     */
    private $password = null;

    /**
     * @var string Authentication info
     * Appears after open connection
     */
    private $basic_auth;

    /**
     * @return bool Check if connected to FileMaker
     */
    public function is_opened()
    {
        return !empty($this->basic_auth);
    }

    /** Set api entry point to FileMaker
     * @param string $path . API entry point
     */
    public function set_entry_point($path)
    {
        $this->entry_point = $path;
    }

    /** Set credentials to FileMaker account
     * @param string $user . Login
     * @param string $password . Password
     */
    public function set_credentials($user, $password)
    {
        $this->user = $user;
        $this->password = $password;
    }

    /**
     * Open connection to FileMaker
     * Response result: {"response":{"token":"f7b67d1df17b9f439f7646e6ceaac3ae6931fba9f9c5e8a6b8ac"},"messages":[{"code":"0","message":"OK"}]}
     */
    public function open_connection()
    {
        if ($this->user == null || $this->entry_point == null) {
            print('Initialize credentials and API entry point first.');
            return;
        }

        try {
            $auth = base64_encode($this->user . ':' . $this->password);
            $header = [
                'Content-Type: application/json',
                'Authorization: Basic ' . $auth
            ];

            $api_url = $this->entry_point . '/sessions';
            $respond_json = $this->send_request($api_url, $header, '{ }', "");
            if (!isset($respond_json) || $respond_json == null) {
                throw new Exception("Wrong response");
            }

            $response = json_decode($respond_json);
            if (empty($response) || !is_object($response) || count($response->messages) == 0) {
                throw new Exception("Wrong response");
            }

            if ($response->messages[0]->code != "0") {
                throw new Exception($response->messages[0]->message);
            }

            $this->basic_auth = $response->response->token;
        } catch (Exception $exception) {
            print('Fatal Error during FileMaker connection: ' . $exception->getMessage());
        }
    }

    /**
     * Close connection to FileMaker
     * It's better to close connection after work. It will auto-destroy on the FileMaker's side, but it's a recommended way.
     */
    public function close_connection()
    {
        if (empty($this->basic_auth)) {
            return;
        }

        $header = [
            'Content-Type: application/json',
        ];

        $api_url = $this->entry_point . '/sessions';
        $this->send_request($api_url . '/' . $this->basic_auth, $header, '', "DELETE");
        $this->basic_auth = null;
        return;
    }

    /** Get all table records
     * FileMaker API can't just return ALL records, so we need to do it in a cycle
     * @param string $table_name . Table name
     * @param string $script . Script in FileMaker to process a table
     * @return array Array of table records
     */
    public function get_all_records($table_name, $script = '')
    {
        if (empty($this->basic_auth)) {
            print('Open FileMaker connection before trying to get records.');
            return [];
        }

        $header = [
            'Content-Type: application/json',
            'Authorization: Bearer ' . $this->basic_auth
        ];

        $offset = 0;
        while (true) {
            try {
                $api_url = $this->entry_point . '/layouts/' . $table_name . '/records?_offset=' . $offset . '&_limit=' . self::$limit . $script;

                $respond_json = $this->send_request($api_url, $header, '', "");
                if (!isset($respond_json) || $respond_json == null) {
                    break;
                }

                $response = json_decode($respond_json);
                if (empty($response) || !is_object($response)) {
                    break;
                }

                if ($response->messages[0]->code != "0") {
                    throw new Exception($response->messages[0]->message);
                }

                yield $response->response->data;
            } catch (Exception $exception) {
                if ($exception->getMessage() == 'Record is missing') {
                    break;
                }

                print('Fatal Error during finding FileMaker content: ' . $exception->getMessage());
            }
        }

        return null;
    }

    /** Get table records based on query
     * @param string $table_name . Table name
     * @param array $query . Query info to get records
     * @return array Array of table records
     */
    public function find_table_records($table_name, $query)
    {
        if (empty($this->basic_auth)) {
            print('Open FileMaker connection before trying to find records.');
            return [];
        }

        try {
            $header = [
                'Content-Type: application/json',
                'Authorization: Bearer ' . $this->basic_auth
            ];
            $api_url = $this->entry_point . '/layouts/' . $table_name . '/_find';

            $respond_json = $this->send_request($api_url, $header, json_encode($query), "");
            if (!isset($respond_json) || $respond_json == null) {
                return [];
            }

            $response = json_decode($respond_json);
            if (empty($response) || !is_object($response)) {
                return [];
            }

            if ($response->messages[0]->code != "0") {
                throw new Exception($response->messages[0]->message);
            }

            return $response->response->data;
        } catch (Exception $exception) {
            if ($exception->getMessage() == 'Record is missing') {
                return null;
            } elseif ($exception->getMessage() == 'No records match the request') {
                return null;
            }

            print('Fatal Error during finding FileMaker content: ' . $exception->getMessage());
        }

        return [];
    }

    /** Update record in FileMaker layout.
     * @param string $table_name . Table name
     * @param array $query_info . Update query
     * @return bool. Updating status. True for success.
     */
    public function update_record($table_name, $query_info)
    {
        if (empty($this->basic_auth)) {
            print('Open FileMaker connection before trying to update record.');
            return false;
        }

        try {
            $header = [
                'Content-Type: application/json',
                'Authorization: Bearer ' . $this->basic_auth
            ];

            $api_url = $this->entry_point . '/layouts/' . $table_name . '/_find';

            $respond_json = $this->send_request($api_url, $header, json_encode($query_info), "");
            if (!isset($respond_json) || $respond_json == null) {
                return false;
            }

            $response = json_decode($respond_json);
            if (empty($response) || !is_object($response)) {
                return false;
            }

            if ($response->messages[0]->code != "0") {
                throw new Exception($response->messages[0]->message);
            }

            return true;
        } catch (Exception $exception) {
            print('Fatal Error during updating FileMaker content: ' . $exception->getMessage());
        }

        return false;
    }

    /** Send request to the website
     * @param string $url . Url
     * @param array $header_params . Header params
     * @param string $post_fields . Request data
     * @param string $customRequest . Custom request params
     * @return mixed|string. Request result
     */
    private function send_request($url, $header_params, $post_fields = '', $customRequest = '')
    {
        $curl = curl_init();
        try {
            curl_setopt($curl, CURLOPT_URL, $url);
            curl_setopt($curl, CURLOPT_HTTPHEADER, $header_params);
            curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
            if (isset($post_fields) && $post_fields != '') {
                curl_setopt($curl, CURLOPT_POST, true);
                curl_setopt($curl, CURLOPT_POSTFIELDS, $post_fields);
            }

            if (isset($customRequest) && $customRequest != '') {
                curl_setopt($curl, CURLOPT_CUSTOMREQUEST, $customRequest);
            }

            curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
            $result = curl_exec($curl);
            curl_close($curl);
            return $result;
        } catch (Exception $exception) {
            curl_close($curl);
            return '';
        }
    }
}
