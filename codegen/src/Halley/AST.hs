{-
   Copyright 2016 Rodrigo Braz Monteiro

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-}
module Halley.AST where

data GenDefinition = ComponentDefinition String [GenEntryDefinition]
                   | SystemDefinition String [GenEntryDefinition]
                   deriving(Show, Eq)

data GenEntryDefinition = MemberList [VariableDeclaration]
                        | FunctionList [FunctionEntry]
                        | Family String [FamilyComponentEntry]
                        | Option String String
                        deriving(Show, Eq)
                        
type VariableDeclaration = (String, String)
type FunctionSignature = ([VariableDeclaration], String)
type FunctionEntry = (String, FunctionSignature)
type FamilyComponentEntry = String
