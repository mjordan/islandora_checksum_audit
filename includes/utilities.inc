<?php

/**
 * @file
 * Helper functions used by this module.
 */

/**
 * Gets FOXML via tuque API-M.
 *
 * @pararm string $pid
 *   The PID of object the FOXML belongs to.
 *
 * @return string|bool
 *   The FOXML, or FALSE if the FOXML is not available.
 */
function islandora_checksum_audit_get_foxml($pid) {
  $foxml = FALSE;
  try {
    $connection = islandora_get_tuque_connection();
    $foxml = $connection->api->m->export($pid, array(
      'format' => 'info:fedora/fedora-system:FOXML-1.1',
      'context' => 'migrate',
      'encoding' => 'UTF-8',
    ));
  }
  catch (Exception $e) {
    drupal_set_message(t(
        "Error retrieving FOXML required to fixity check data: @message",
        array("@message" => $e->getMessage())), 'error');
    watchdog(
      "islandora_checksum_audit",
      "Error retrieving FOXML for !pid required to fixity check data: @message",
      array('!pid' => $pid, '@message' => $e->getMessage),
      WATCHDOG_ERROR
    );
  }

  return $foxml;
}

/**
 * Parses out the fixity check events from the FOXML.
 *
 * @pararm string $pid
 *   The PID of object the FOXML belongs to.
 *
 * @return array
 *   An associative array with DSIDs as the top-level keys, each of which
 *   contains an array of 'valid' and 'invalid' audit record IDs with
 *   associated date and event values.
 */
function islandora_checksum_audit_get_fixity_checks($foxml) {
  $dom = new DOMDocument();
  $dom->loadXML($foxml);

  $fixity_checks = array();
  foreach ($dom->getElementsByTagNameNS('info:fedora/fedora-system:def/audit#', 'record') as $record) {
    $id = $record->getAttributeNode('ID');
    foreach ($record->childNodes as $child_node) {
      if ($child_node->localName == 'date') {
        $date = $child_node->nodeValue;
      }
      if ($child_node->localName == 'justification') {
        $justification_string = $child_node->nodeValue;
      }

      if (isset($justification_string) && strpos($justification_string, 'eventType=fixity check')) {
        $justification_details = islandora_checksum_audit_parse_justification($justification_string);
        $dsid = $justification_details['file'];

        if (strpos($justification_details['event'], 'checksum validated')) {
          $fixity_checks[$dsid]['valid'][$id->nodeValue]['date'] = $date;
          $fixity_checks[$dsid]['valid'][$id->nodeValue]['event'] = $justification_details['event'];
        }
        else {
          $fixity_checks[$dsid]['invalid'][$id->nodeValue]['date'] = $date;
          $fixity_checks[$dsid]['invalid'][$id->nodeValue]['event'] = $justification_details['event'];
        }

      }
    }
  }
  return $fixity_checks;
}

/**
 * Parses out the data we want from the <justification> value.
 *
 * @pararm string $string
 *   The value of the <justification> element.
 *
 * @return array
 *   An associative array containing the DSID and the substring we want.
 */
function islandora_checksum_audit_parse_justification($string) {
  $justification_parts = explode(';', $string);
  $file = explode('+', $justification_parts[0]);
  return array('file' => $file[1], 'event' => $justification_parts[2]);
}
