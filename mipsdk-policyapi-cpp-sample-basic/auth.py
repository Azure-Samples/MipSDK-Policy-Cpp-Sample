#
# Copyright (c) Microsoft Corporation.
# All rights reserved.
#
# This code is licensed under the MIT License.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# make sure to run pip install adal, first. 
# for non-windows setup, review https://github.com/AzureAD/azure-activedirectory-library-for-python/wiki/ADAL-basics

import getopt
import sys
import json
import re
import msal

def printUsage():
  print('auth.py -k <app key> -a <authority> -r <resource> -c <clientId> -t <tenantId>')

def main(argv):
  try:
    options, args = getopt.getopt(argv, 'hk:a:r:t:c:')
  except getopt.GetoptError:
    printUsage()
    sys.exit(-1)

  appKey = ''
  authority = ''
  resource = ''
  clientId = ''
  tenantId = ''
    
  for option, arg in options:
    if option == '-h':
      printUsage()
      sys.exit()
    elif option == '-k':
      appKey = arg
    elif option == '-a':
      authority = arg
    elif option == '-r':
      resource = arg
    elif option == '-c':
      clientId = arg
    elif option == '-t':
      tenantId = arg

  if appKey == '' or authority == '' or resource == '' or clientId == '' or tenantId == '':
    printUsage()
    sys.exit(-1)

  # ONLY FOR DEMO PURPOSES AND MSAL FOR PYTHON
  # This shouldn't be required when using proper auth flows in production.  
  if authority.find('common') > 1:
    authority = authority.split('/common')[0] + "/" + tenantId
  
     
  # Create a preferably long-lived app instance which maintains a token cache.
  app = msal.ConfidentialClientApplication(
    clientId, authority=authority,
    client_credential=appKey,
    # token_cache=...  # Default cache is in memory only.
                       # You can learn how to use SerializableTokenCache from
                       # https://msal-python.readthedocs.io/en/latest/#msal.SerializableTokenCache
    )

  result = None
 
  if resource.endswith('/'):
    resource += ".default"    
  else:
    resource += "/.default"
  
  scope = [resource]

  result = app.acquire_token_silent(scope, account=None)
  
  if not result:
    #logging.info("No suitable token exists in cache. Let's get a new one from AAD.")
    result = app.acquire_token_for_client(scopes=scope)
  
  if "access_token" in result:
    print(result['access_token'])

  else:
    print(result.get("error"))
    print(result.get("error_description"))
    print(result.get("correlation_id"))

if __name__ == '__main__':  
  main(sys.argv[1:])
