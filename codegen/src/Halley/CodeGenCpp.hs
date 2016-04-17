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
module Halley.CodeGenCpp
( generateCodeCpp
) where

import Halley.CodeGen
import Halley.AST
import Halley.SemanticAnalysis
import Data.List
import Data.Char
import qualified Data.Map as Map

---------------------


generateCodeCpp :: CodeGenData -> [GeneratedSource]
generateCodeCpp gen = comps ++ sys
    where
        comps = concat (map (genComponent) (components gen))
        sys = concat (map (genSystem) (systems gen))


---------------------


genVariable :: VariableData -> String
genVariable var = genType (variableType var) ++ " " ++ (variableName var)

genType :: VariableTypeData -> String
genType t = (if Halley.SemanticAnalysis.const t then "const " else "") ++ typeName t

genFunctionDeclaration :: FunctionData -> String
genFunctionDeclaration f = concat [genType (returnType f)
                                  ," "
                                  ,(functionName f)
                                  ," ("
                                  ,");"]

genMember :: VariableData -> String
genMember var = concat ["    "
                       ,genVariable var
                       ,";"]

genFamilyMember :: VariableTypeData -> String
genFamilyMember varType = "        " ++ (genType varType) ++ "* const " ++ (removeSuffix "Component" $ lowerFirst $ typeName varType) ++ ";"

---------------------


genComponent :: ComponentData -> [GeneratedSource]
genComponent compData = [ GeneratedSource { filename = basePath ++ ".h", code = header } ]
    where
        name = componentName compData
        basePath = "cpp/components/" ++ (makeUnderscores name)
        header = intercalate "\n" headerCode
        headerCode = ["#pragma once"
                     ,""
                     ,"//#include \"component.h\""
                     ,"class " ++ name ++ " : public Component {"
                     ,"public:"
                     ,"    constexpr static int componentIndex = " ++ (show $ componentIndex compData) ++ ";"
                     ,""]
                     ++ memberList ++
                     [""]
                     ++ functionList ++
                     ["};"
                     ,""]
        memberList = map (genMember) (members compData)
        functionList = map (genFunctionDeclaration) (functions compData)

genSystem :: SystemData -> [GeneratedSource]
genSystem sysData = [GeneratedSource{filename = basePath ++ ".h", code = header}]
    where
        baseSystemClass = "System"
        name = systemName sysData
        fams = families sysData
        basePath = "cpp/systems/" ++ (makeUnderscores name)
        header = intercalate "\n" headerCode
        headerCode = ["#pragma once"
                     ,""
                     ,"//#include \"system.h\""]
                     ++ componentIncludes ++
                     [""
                     ,"// Generated file; do not modify."
                     ,"class " ++ name ++ " : public Halley::" ++ baseSystemClass ++ " {"]
                     ++ familyTypeDecls
                     ++ familyBindings ++
                     [""
                     ,baseMethod
                     ,""
                     ,"protected:"
                     ,stubMethod
                     ,""
                     ,"public:"
                     ,"    " ++ name ++ "() : " ++ baseSystemClass ++ "({" ++ familyInitializers ++ "}) {}"
                     ,""
                     ,"};"
                     ,""]
        familyTypeDecls = concat $ map (familyTypeDecl) $ fams
        familyTypeDecl fam = ["    class " ++ fName ++ " {"
                             ,"    public:"
                             ,"        const Halley::EntityId entityId;"
                             ,""]
                             ++ memberList ++
                             [""
                             ,"        using Type = Halley::FamilyType<" ++ typeList ++ ">;"
                             ,"    };"
                             ,""]
                             where
                                memberList = map (genFamilyMember) (familyComponents fam)
                                typeList = intercalate ", " $ map (typeName) (familyComponents fam)
                                fName = familyTypeName fam
        familyBindings = map (familyBinding) $ fams
        familyBinding fam = "    Halley::FamilyBinding<" ++ (familyTypeName fam) ++ "> " ++ (familyName fam) ++ "Family;"
        familyTypeName fam = (upperFirst $ familyName fam) ++ "Family"
        componentIncludes = map (\c -> "#include \"../components/" ++ (makeUnderscores $ typeName c) ++ ".h\"") components
            where
                components = nub $ concat $ map (familyComponents) fams
        familyInitializers = intercalate ", " $ map (\f -> '&' : familyName f ++ "Family") fams
        options = configurationOptions sysData
        methodSignature = "" -- TODO: generate signature based on strategy
        (baseMethod, stubMethod) = (makeBase, makeStub)
            where
                strategy = Map.findWithDefault "default" "strategy" options
                method = Map.findWithDefault "update" "method" options
                isUpdate = method == "update"
                isDefault = strategy == "default"
                makeBase = "    void " ++ method ++ "Base(" ++ methodSignature ++ ") override { " ++ baseBody ++ " }"
                baseBody = if isDefault then method ++ "(p);" else "invoke" ++ (upperFirst strategy) ++ "(this, &" ++ name ++ "::" ++ method ++ ", p, mainFamily);"
                makeStub = "    void " ++ method ++ "(" ++ stubSignature ++ ")" ++ methodConst ++ "; // Implement me"
                methodConst = if isUpdate then "" else " const"
                methodOverride = if isDefault then " override" else ""
                methodSignature = if isUpdate then "Halley::Time p" else "Halley::Painter& p"
                stubSignature = methodSignature ++ if isDefault then "" else ", " ++ individualType ++ "& entity"
                individualType = familyTypeName (head (filter (\f -> familyName f == "main") fams))


---------------------


makeUnderscores :: String -> String
makeUnderscores [] = []
makeUnderscores (c:cs) = (toLower c) : (concat $ map (processChar) cs)
    where
        processChar c' = if isAsciiUpper c' then '_' : [toLower c'] else [c']

upperFirst :: String -> String
upperFirst [] = []
upperFirst (c:cs) = (toUpper c) : cs

lowerFirst :: String -> String
lowerFirst [] = []
lowerFirst (c:cs) = (toLower c) : cs

removeSuffix :: String -> String -> String
removeSuffix suffix str = if isSuffixOf suffix str then take (length str - length suffix) str else str
